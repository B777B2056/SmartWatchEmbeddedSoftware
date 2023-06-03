#include "adxl345.h"
#include "main.h"
#include "i2c.h"
#include "rtc.h"
#include "cmsis_os.h"
#include "adxl345_hal.h"
#include <stdbool.h>
#include <math.h>

#define ACC_BUFFER_LEN  8
#define GRAVIT_ACC      1

uint32_t step_count = 0;
uint8_t acc_buf_cur_idx = 0;
static float acc_buffer[ACC_BUFFER_LEN];
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

void ADXL345_Init()
{
  osDelay(1000);
  ADXL345_Driver_Init(&hi2c2);
}

void ADXL345_DoStepCnt()
{
  // If the time has passed 0 o'clock, reset the number of steps
  if (IsNewDayStarted())  ADXL345_ResetStepCnt();
  // Count steps
  float accX = 0.0; float accY = 0.0; float accZ = 0.0;
  ADXL345_Driver_Axis_Data(&accX, &accY, &accZ);
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
