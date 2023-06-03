#include "oled.h"
#include "cmsis_os.h"
#include "i2c.h"
#include "rtc.h"
#include <stdio.h>
#include <string.h>
#include "oled_hal.h"
#include "oledfont.h"
#include "oledimg.h"

#define ASCII_SYMBOL_WIDTH 8
#define CHINESE_SYMBOL_WIDTH 16
#define OLED_CENTERED_POS(CHINESE_N, ASCII_N) \
  (((OLED_WIDTH)-((ASCII_SYMBOL_WIDTH)*(ASCII_N))-((CHINESE_SYMBOL_WIDTH)*(CHINESE_N)))/2)

PageType current_page;
extern osMutexId pageSwitchMutexHandle;

void OLED_Init()
{
  osDelay(1000);
  OLED_Driver_Init(&hi2c1);
  OLED_Clear();
  OLED_Driver_SetPos(0, 0);
  OLED_SetCurrentPage(DATE_PAGE);
}

void OLED_Clear()
{
  OLED_Driver_ClearTargetRow(0);
}

void OLED_DisplayOn()
{
	OLED_Driver_WriteCmd(0x8D);
	OLED_Driver_WriteCmd(0x14);
	OLED_Driver_WriteCmd(0xAF);
}

void OLED_DisplayOff()
{
	OLED_Driver_WriteCmd(0x8D);
	OLED_Driver_WriteCmd(0x10);
	OLED_Driver_WriteCmd(0xAF);
}

void OLED_ShowStartup()
{
  OLED_Driver_ShowChinese(0, 0, STARTUP_CHINESE);
}

void OLED_ShowShutdown()
{
  OLED_Driver_ShowChinese(0, 0, SHUTDOWN_CHINESE);
}

void OLED_ShowDate()
{
  // 获取时间
  RTC_DateTypeDef date;
  RTC_TimeTypeDef time;
  HAL_RTC_GetDate(&hrtc, &date, RTC_FORMAT_BIN);
  HAL_RTC_GetTime(&hrtc, &time, RTC_FORMAT_BIN);
  // 显示日期：年-月-日
  char date_str[32];
  snprintf(date_str, sizeof(date_str), "20%02d-%02d-%02d", date.Year, date.Month, date.Date);
  OLED_Driver_ShowAsciiString(OLED_CENTERED_POS(0, 10), 1, date_str);
  // 显示星期：星期X
  uint8_t week_start_pos = OLED_CENTERED_POS(3, 0);
  OLED_Driver_ShowChinese(week_start_pos, 3, WEEK_CHINESE);
  OLED_Driver_ShowChinese(week_start_pos+2*16, 3, WEEK_DAY_CHINESE[date.WeekDay-1]);
  // 显示时间：时:分:秒
  char time_str[16];
  snprintf(time_str, sizeof(time_str), "%02d:%02d:%02d", time.Hours, time.Minutes, time.Seconds);
  OLED_Driver_ShowAsciiString(OLED_CENTERED_POS(0, 8), 5, time_str);
}

static void OLED_ShowTimeOnTop()
{
  RTC_TimeTypeDef time;
  HAL_RTC_GetTime(&hrtc, &time, RTC_FORMAT_BIN);
  char time_str[8];
  snprintf(time_str, sizeof(time_str), "%02d:%02d", time.Hours, time.Minutes);
  OLED_Driver_ShowAsciiString(0, 0, time_str);
}

void OLED_ShowHealthy(int32_t heart_rate, int32_t spo2)
{
  OLED_ShowTimeOnTop();
  char hr_str[10];
  snprintf(hr_str, sizeof(hr_str), "%3ld", heart_rate);
  char spo2_str[6];
  snprintf(spo2_str, sizeof(spo2_str), "%3ld%%", spo2);
  // 显示心率
  {
    uint8_t hr_start_pos = OLED_CENTERED_POS(3, 5);
    OLED_Driver_ShowImage(hr_start_pos, 3, HEART_RATE_IMAGE);
    OLED_Driver_ShowAsciiString(hr_start_pos+16+8, 3, hr_str);
    OLED_Driver_ShowAsciiString(hr_start_pos+16+8*5, 3, "BPM");
  }
  // 显示血氧
  {
    uint8_t spo2_start_pos = OLED_CENTERED_POS(2, 5);
    OLED_Driver_ShowChinese(spo2_start_pos, 6, HEALTHY_CHINESE[2]);
    OLED_Driver_ShowAsciiString(spo2_start_pos+2*16, 6, spo2_str);
  }
}

static uint16_t CalculateCalories(uint16_t step)
{
  return step * 0.063;
}

void OLED_ShowStepCnt(uint16_t step)
{
  OLED_ShowTimeOnTop();
  char step_str[6];
  snprintf(step_str, sizeof(step_str), "%5hu", step);
  char calories_str[8];
  snprintf(calories_str, sizeof(calories_str), "%4d", CalculateCalories(step));
  // 显示计步：今日步数xxxx
  {
    uint8_t sc_start_pos = OLED_CENTERED_POS(4, 4);
    OLED_Driver_ShowChinese(sc_start_pos, 3, STEP_CNT_CHINESE);
    OLED_Driver_ShowAsciiString(sc_start_pos+4*16, 3, step_str);
  }
  // 显示卡路里：xxx大卡
  {
    uint8_t calories_start_pos = OLED_CENTERED_POS(2, 4);
    OLED_Driver_ShowAsciiString(calories_start_pos, 6, calories_str);
    OLED_Driver_ShowChinese(calories_start_pos+4*8, 6, CALORIES_UNIT_CHINESE);
  }
}

void OLED_ShowComingCall(const char* phone_number, bool isAcceptCall)
{
  OLED_Driver_ShowChinese(OLED_CENTERED_POS(2, 0), 0, COMING_CALL_CHINESE); 
  OLED_Driver_ShowAsciiString(OLED_CENTERED_POS(0, 11), 2, phone_number);
  OLED_Driver_ClearTargetRow(6);
  if (isAcceptCall)
  {
    OLED_Driver_ShowChinese(0, 6, CALL_ACCEPT_CHINESE);
    OLED_Driver_ShowChinese(2*16, 6, ACCEPT_SYMBOL);
    OLED_Driver_ShowChinese(6*16, 6, CALL_REJECT_CHINESE);
  }
  else
  {
    OLED_Driver_ShowChinese(0, 6, CALL_ACCEPT_CHINESE);
    OLED_Driver_ShowChinese(5*16, 6, CALL_REJECT_CHINESE);
    OLED_Driver_ShowChinese(7*16, 6, ACCEPT_SYMBOL);
  }
}

void OLED_SetCurrentPage(PageType page)
{
  OLED_Clear();
  osMutexWait(pageSwitchMutexHandle, osWaitForever);
  current_page = page;
  osMutexRelease(pageSwitchMutexHandle);
}

PageType OLED_CurrentPage()
{
  osMutexWait(pageSwitchMutexHandle, osWaitForever);
  PageType tmp = current_page;
  osMutexRelease(pageSwitchMutexHandle);
  return tmp;
}

void OLED_GoNextPage()
{
  OLED_Clear();
  osMutexWait(pageSwitchMutexHandle, osWaitForever);
  current_page = (current_page+1) % (PAGE_N-1); // Coming call page not show in normal
  osMutexRelease(pageSwitchMutexHandle);
}
