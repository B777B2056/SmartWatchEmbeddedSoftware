#include "adxl345.h"
#include "cmsis_os.h"
#include "main.h"
#include "i2c.h"
#include "rtc.h"
#include "adxl345_hal.h"
#include <stdbool.h>
#include <math.h>

#define GRAVIT_ACC 1

extern osMutexId stepCntGetterMutexHandle;

/* 判断是否已经过了00:00，用于步数重置 */
static bool IsNewDayStarted()
{
  // 获取时间
  RTC_TimeTypeDef time;
  HAL_RTC_GetTime(&hrtc, &time, RTC_FORMAT_BIN);
  // 判断时间是否过了00:00
  return (0 == time.Hours) && (0 == time.Minutes);
}

static void ADXL345_ResetStepCnt(adxl345_t* obj)
{
  osMutexWait(stepCntGetterMutexHandle, osWaitForever);
  obj->step_count = 0;
	osMutexRelease(stepCntGetterMutexHandle);
}

static float CalculateACCStd(adxl345_t* obj)
{
  float sum = 0.0, mean, std = 0.0;
  uint8_t i;
  for(i = 0; i < ACC_BUFFER_LEN; ++i)
    sum += obj->acc_buffer[i];
  mean = sum / ACC_BUFFER_LEN;
  for(i = 0; i < ACC_BUFFER_LEN; ++i)
    std += ((obj->acc_buffer[i] - mean) * (obj->acc_buffer[i] - mean));
  return sqrt(std / ACC_BUFFER_LEN);
}

void ADXL345_Init(adxl345_t* obj)
{
  obj->step_count = obj->acc_buf_cur_idx = 0;
  ADXL345_Driver_Init(&hi2c2);
}

void ADXL345_DoStepCnt(adxl345_t* obj)
{
  // If the time has passed 0 o'clock, reset the number of steps
  if (IsNewDayStarted())
  {
    ADXL345_ResetStepCnt(obj);
    return;
  }
  // Count steps
  float accX = 0.0; float accY = 0.0; float accZ = 0.0;
  ADXL345_Driver_Axis_Data(&accX, &accY, &accZ);
  float acc = sqrt(accX*accX + accY*accY + accZ*accZ) - GRAVIT_ACC;
  // Calculate acc std as peak threshold
  float peak_threshold = CalculateACCStd(obj);
  // Fill acc buffer
  if (obj->acc_buf_cur_idx < ACC_BUFFER_LEN)
  {
    obj->acc_buffer[obj->acc_buf_cur_idx++] = acc;
    if (ACC_BUFFER_LEN - 1 == obj->acc_buf_cur_idx)  obj->acc_buf_cur_idx = 0;
    else  return;
  }
  // Calculate peak number(higher than acc array std)
  uint8_t i, peak_cnt = 0;
  for (i = 1; i < ACC_BUFFER_LEN - 1; ++i)
  {
    if (obj->acc_buffer[i] <= peak_threshold)  continue;
    if ((obj->acc_buffer[i-1] < obj->acc_buffer[i]) && (obj->acc_buffer[i] > obj->acc_buffer[i+1]))
    {
      if ((obj->acc_buffer[i] - obj->acc_buffer[i-1] > 0.1) && (obj->acc_buffer[i] - obj->acc_buffer[i+1] > 0.1))
        ++peak_cnt;
    }
  }
  // Calculate step number
  osMutexWait(stepCntGetterMutexHandle, osWaitForever);
  obj->step_count += peak_cnt;
  osMutexRelease(stepCntGetterMutexHandle);
}

uint32_t ADXL345_GetSteps(adxl345_t* obj)
{
	osMutexWait(stepCntGetterMutexHandle, osWaitForever);
  uint32_t tmp = obj->step_count;
	osMutexRelease(stepCntGetterMutexHandle);
  return tmp;
}

uint16_t ADXL345_GetCalories(adxl345_t* obj) 
{
  return ADXL345_GetSteps(obj) * 0.046;
}
