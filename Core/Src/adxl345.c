#include "adxl345.h"
#include "main.h"
#include "i2c.h"
#include "rtc.h"
#include "cmsis_os.h"
#include <stdbool.h>
#include <math.h>

#define DEVID		        0X00 	// 器件ID
#define THRESH_TAP		  0X1D   	// 敲击阀值寄存器
#define OFSX			      0X1E
#define OFSY			      0X1F
#define OFSZ			      0X20
#define DUR				      0X21
#define Latent			    0X22
#define Window  		    0X23 
#define THRESH_ACT		  0X24	// 运动阈值寄存器
#define THRESH_INACT	  0X25 	// 静止阈值寄存器
#define TIME_INACT		  0X26	// 静止时间			比例1 sec /LSB
#define ACT_INACT_CTL	  0X27	// 启用运动/静止检测
#define THRESH_FF		    0X28	// 自由下落阈值	建议采用300 mg与600 mg(0x05至0x09)之间的值 比例62.5 mg/LSB
#define TIME_FF			    0X29 	// 自由下落时间	建议采用100 ms与350 ms(0x14至0x46)之间的值 比例5ms/LSB
#define TAP_AXES		    0X2A  
#define ACT_TAP_STATUS  0X2B 
#define BW_RATE			    0X2C 
#define POWER_CTL		    0X2D 
#define INT_ENABLE		  0X2E	// 设置中断配置
#define INT_MAP			    0X2F
#define INT_SOURCE  	  0X30
#define DATA_FORMAT	    0X31
#define DATA_X0			    0X32
#define DATA_X1			    0X33
#define DATA_Y0			    0X34
#define DATA_Y1			    0X35
#define DATA_Z0			    0X36
#define DATA_Z1			    0X37
#define FIFO_CTL		    0X38
#define FIFO_STATUS		  0X39

#define ACC_BUFFER_LEN  8
#define GRAVIT_ACC      1
#define ADXL345_ADDR    (0X53 << 1)

extern osMutexId stepCntMutexHandle;

/* 判断是否已经过了00:00，用于步数重置 */
static bool IsNewDayStarted()
{
  // 获取时间
  RTC_TimeTypeDef time;
  HAL_RTC_GetTime(&hrtc, &time, RTC_FORMAT_BIN);
  // 判断时间是否过了00:00
  return (0 == time.Hours) && (0 == time.Minutes);
}

static void ADXL345_Write(uint8_t addr, uint8_t value)
{
	if (addr > 63)  addr = 63;
  addr &= ~(0x80);
	HAL_I2C_Mem_Write(&hi2c2, ADXL345_ADDR, addr, I2C_MEMADD_SIZE_8BIT, &value, 1, 0xFF);
}


static void ADXL345_Read(uint8_t addr, uint8_t* vvalue)
{
	if (addr > 63)  addr = 63;
	addr &= ~(0x40);
  addr |= (0x80);
  HAL_I2C_Mem_Read(&hi2c2, ADXL345_ADDR, addr, I2C_MEMADD_SIZE_8BIT, vvalue, 1, 0xFF);
}

void ADXL345_Init()
{
  osDelay(1000);
  uint8_t id;
  HAL_I2C_Mem_Read(&hi2c2, ADXL345_ADDR, DEVID,I2C_MEMADD_SIZE_8BIT, &id, 1, 0xff);
  if (0XE5 == id)
  {
    ADXL345_Write(INT_ENABLE, 0x00);
    ADXL345_Write(DATA_FORMAT, 0x0B);
    ADXL345_Write(BW_RATE, 0x08);
    ADXL345_Write(POWER_CTL, 0x08);
    ADXL345_Write(OFSX, 0X00);
    ADXL345_Write(OFSY, 0X00);
    ADXL345_Write(OFSZ, 0X00);
  }
}

static void ADXL345_Axis_Data(float* acc_x, float* acc_y, float* acc_z)
{
	uint8_t l, h;
	{
    ADXL345_Read(DATA_X0, &l);
    ADXL345_Read(DATA_X1, &h);
    *acc_x =  (float)((int16_t)(((uint16_t)h << 8) | l)) / 256.0f;
  }
  {
    ADXL345_Read(DATA_Y0, &l);
    ADXL345_Read(DATA_Y1, &h);
    *acc_y =  (float)((int16_t)(((uint16_t)h << 8) | l)) / 256.0f;
  }
  {
    ADXL345_Read(DATA_Z0, &l);
    ADXL345_Read(DATA_Z1, &h);
    *acc_z =  (float)((int16_t)(((uint16_t)h << 8) | l)) / 256.0f;
  }
}

uint32_t step_count = 0;
uint8_t acc_buf_cur_idx = 0;
static float acc_buffer[ACC_BUFFER_LEN];

static void ADXL345_ResetStepCnt()
{
  osMutexWait(stepCntMutexHandle, osWaitForever);
  step_count = 0;
	osMutexRelease(stepCntMutexHandle);
}

static float CalculateACCStd()
{
  float sum = 0.0, mean, std = 0.0;
  uint8_t i;
  for(i = 0; i < ACC_BUFFER_LEN; ++i)
    sum += acc_buffer[i];
  mean = sum / ACC_BUFFER_LEN;
  for(i = 0; i < ACC_BUFFER_LEN; ++i)
    std += ((acc_buffer[i] - mean) * (acc_buffer[i] - mean));
  return sqrt(std / ACC_BUFFER_LEN);
}

void ADXL345_DoStepCnt()
{
  // If the time has passed 0 o'clock, reset the number of steps
  if (IsNewDayStarted())  ADXL345_ResetStepCnt();
  // Count steps
  float accX = 0.0; float accY = 0.0; float accZ = 0.0;
  ADXL345_Axis_Data(&accX, &accY, &accZ);
  float acc = sqrt(accX*accX + accY*accY + accZ*accZ) - GRAVIT_ACC;
  // Calculate acc std as peak threshold
  float peak_threshold = CalculateACCStd();
  // Fill acc buffer
  if (acc_buf_cur_idx < ACC_BUFFER_LEN)
  {
    acc_buffer[acc_buf_cur_idx++] = acc;
    if (ACC_BUFFER_LEN - 1 == acc_buf_cur_idx)  acc_buf_cur_idx = 0;
    else  return;
  }
  // Calculate peak number(higher than acc array std)
  uint8_t i, peak_cnt = 0;
  for (i = 1; i < ACC_BUFFER_LEN - 1; ++i)
  {
    if (acc_buffer[i] <= peak_threshold)  continue;
    if ((acc_buffer[i-1] < acc_buffer[i]) && (acc_buffer[i] > acc_buffer[i+1]))
    {
      if ((acc_buffer[i] -acc_buffer[i-1] > 0.1) && (acc_buffer[i] - acc_buffer[i+1] > 0.1))
        ++peak_cnt;
    }
  }
  // Calculate step number
  osMutexWait(stepCntMutexHandle, osWaitForever);
  step_count += peak_cnt;
  osMutexRelease(stepCntMutexHandle);
}

uint32_t ADXL345_GetSteps()
{
	osMutexWait(stepCntMutexHandle, osWaitForever);
  uint32_t tmp = step_count;
	osMutexRelease(stepCntMutexHandle);
  return tmp;
}
