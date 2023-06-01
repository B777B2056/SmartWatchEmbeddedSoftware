#include "adxl345.h"
#include "main.h"
#include "spi.h"
#include "cmsis_os.h"

#define REPEAT_CNT      5       // 初始化时的重试次数
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

#define DATA_X_START    DATA_X0
#define DATA_Y_START    DATA_Y0
#define DATA_Z_START    DATA_Z0

extern osMutexId stepCntMutexHandle;

static void NSS_Low()
{
  HAL_GPIO_WritePin(SPI_NSS_GPIO_Port, SPI_NSS_Pin, GPIO_PIN_RESET);
}

static void NSS_High()
{
  HAL_GPIO_WritePin(SPI_NSS_GPIO_Port, SPI_NSS_Pin, GPIO_PIN_SET);
}

static void ADXL345_Write(uint8_t addr, uint8_t value)
{
	addr &= 0x3F;
	NSS_Low();
	HAL_SPI_Transmit(&hspi1, &addr, 1, 10);
	HAL_SPI_Transmit(&hspi1, &value, 1, 10);
	NSS_High();
}


static void ADXL345_Read(uint8_t addr, uint8_t* vvalue)
{
	addr &= 0x3F;	
	addr |= (0x80);
	NSS_Low();
	HAL_SPI_Transmit(&hspi1, &addr, 1, 10);
	HAL_SPI_Receive(&hspi1, vvalue, 1, 10);
	NSS_High();
}

static uint8_t ADXL345_GetID()
{
	uint8_t result = 0;
	ADXL345_Read(DEVID, &result);
	return result;
}

void ADXL345_Init()
{
  uint8_t repeat = 0;
  for (; repeat < REPEAT_CNT; ++repeat)
  {
      if (0xE5 == ADXL345_GetID())   break;
  }
  if (REPEAT_CNT == repeat)    return;
  	ADXL345_Write(INT_ENABLE, 0x00);
	ADXL345_Write(DATA_FORMAT, 0x0B);
	ADXL345_Write(BW_RATE, 0x1A);
	ADXL345_Write(POWER_CTL, 0x08);
	ADXL345_Write(INT_ENABLE, 0x14);
}

static float ADXL345_Axis_Data(uint8_t addrl)
{
	uint8_t addrh = addrl + 0x01;
	uint8_t l, h;

	ADXL345_Read(addrl, &l);
	ADXL345_Read(addrh, &h);

	return (float)(((uint16_t)h << 8) + l);
}

static float ADXL345_AxisX_Data()
{
  return ADXL345_Axis_Data(DATA_X_START);
}

static float ADXL345_AxisY_Data()
{
  return ADXL345_Axis_Data(DATA_Y_START);
}

static float ADXL345_AxisZ_Data()
{
  return ADXL345_Axis_Data(DATA_Z_START);
}

static uint32_t step_count = 0;

void ADXL345_DoStepCnt()
{
  // Count steps
  float accZ = ADXL345_AxisZ_Data();
  if (accZ < 950.0f || accZ  > 1000.0f)
  {
		osMutexWait(stepCntMutexHandle, osWaitForever);
    ++step_count;
		osMutexWait(stepCntMutexHandle, osWaitForever);
  }
}

uint32_t ADXL345_GetSteps()
{
	osMutexWait(stepCntMutexHandle, osWaitForever);
  uint32_t tmp = step_count;
	osMutexWait(stepCntMutexHandle, osWaitForever);
  return tmp;
}
