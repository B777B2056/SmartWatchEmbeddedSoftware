#include "oled.h"
#include "cmsis_os.h"
#include "i2c.h"
#include "rtc.h"
#include <stdio.h>
#include <string.h>
#include "oledfont.h"
#include "oledimg.h"

#define OLED_ADDR 0x78

#define OLED_WIDTH 128
#define OLED_HEIGHT 32

#define ASCII_SYMBOL_WIDTH 8
#define CHINESE_SYMBOL_WIDTH 16
#define OLED_CENTERED_POS(CHINESE_N, ASCII_N) \
  (((OLED_WIDTH)-((ASCII_SYMBOL_WIDTH)*(ASCII_N))-((CHINESE_SYMBOL_WIDTH)*(CHINESE_N)))/2)

#define CHECK_PAGE_AND_GO_NEXT(x, y)  \
    if (x > 128)  \
    { \
      x = 0;  \
      y += 2; \
    }

PageType current_page;
extern osMutexId pageSwitchMutexHandle;

static void OLED_WriteCmd(uint8_t cmd)
{
  HAL_I2C_Mem_Write(&hi2c1, OLED_ADDR, 0x00, I2C_MEMADD_SIZE_8BIT, &cmd, 1, 0x100);
}

static void OLED_WriteChar(uint8_t data)
{
  HAL_I2C_Mem_Write(&hi2c1, OLED_ADDR, 0x40, I2C_MEMADD_SIZE_8BIT, &data, 1, 0x100);
}

static void OLED_SetPos(uint8_t x, uint8_t y)
{
	OLED_WriteCmd(0xb0 + y);
	OLED_WriteCmd(((x & 0xf0) >> 4) | 0x10);
	OLED_WriteCmd((x & 0x0f) | 0x00);
}

static void OLED_ClearTargetRow(uint8_t row)
{
  uint8_t i, n;
  for (i = row; i < 8; ++i)
  {
    OLED_WriteCmd(0xb0 + i);
		OLED_WriteCmd(0x00);
		OLED_WriteCmd(0x10);
		for (n = 0; n < 128; n++)
		{
			OLED_WriteChar(0x00);
		}
  }
}

void OLED_Init()
{
  osDelay(1000);
  OLED_WriteCmd(0xAE); //display off
  OLED_WriteCmd(0x20); //Set Memory Addressing Mode
  OLED_WriteCmd(0x10); //00,Horizontal Addressing Mode;01,Vertical Addressing Mode;10,Page Addressing Mode (RESET);11,Invalid
  OLED_WriteCmd(0xb0); //Set Page Start Address for Page Addressing Mode,0-7
  OLED_WriteCmd(0xc8); //Set COM Output Scan Direction
  OLED_WriteCmd(0x00); //---set low column address
  OLED_WriteCmd(0x10); //---set high column address
  OLED_WriteCmd(0x40); //--set start line address
  OLED_WriteCmd(0x81); //--set contrast control register
  OLED_WriteCmd(0xff); //亮度调节 0x00~0xff
  OLED_WriteCmd(0xa1); //--set segment re-map 0 to 127
  OLED_WriteCmd(0xa6); //--set normal display
  OLED_WriteCmd(0xa8); //--set multiplex ratio(1 to 64)
  OLED_WriteCmd(0x3F); //
  OLED_WriteCmd(0xa4); //0xa4,Output follows RAM content;0xa5,Output ignores RAM content
  OLED_WriteCmd(0xd3); //-set display offset
  OLED_WriteCmd(0x00); //-not offset
  OLED_WriteCmd(0xd5); //--set display clock divide ratio/oscillator frequency
  OLED_WriteCmd(0xf0); //--set divide ratio
  OLED_WriteCmd(0xd9); //--set pre-charge period
  OLED_WriteCmd(0x22); //
  OLED_WriteCmd(0xda); //--set com pins hardware configuration
  OLED_WriteCmd(0x12);
  OLED_WriteCmd(0xdb); //--set vcomh
  OLED_WriteCmd(0x20); //0x20,0.77xVcc
  OLED_WriteCmd(0x8d); //--set DC-DC enable
  OLED_WriteCmd(0x14); //
  OLED_WriteCmd(0xaf); //--turn on oled panel
  OLED_Clear();
  OLED_SetPos(0, 0);
  OLED_SetCurrentPage(DATE_PAGE);
}

void OLED_Clear()
{
  OLED_ClearTargetRow(0);
}

void OLED_DisplayOn()
{
	OLED_WriteCmd(0x8D);
	OLED_WriteCmd(0x14);
	OLED_WriteCmd(0xAF);
}

void OLED_DisplayOff()
{
	OLED_WriteCmd(0x8D);
	OLED_WriteCmd(0x10);
	OLED_WriteCmd(0xAF);
}

static void OLED_ShowAsciiChar(uint8_t start_x, uint8_t start_y, unsigned char ch)
{
  unsigned char i, c = ch - ' ';
  CHECK_PAGE_AND_GO_NEXT(start_x, start_y);
  OLED_SetPos(start_x, start_y);
  for (i = 0; i < 8; ++i)
  {
      OLED_WriteChar(F8X16[c*16 + i]);
  }
  OLED_SetPos(start_x, start_y + 1);
  for (i = 0; i < 8; ++i)
  {
      OLED_WriteChar(F8X16[c*16 + 8 + i]);
  }
}

static void OLED_ShowAsciiString(uint8_t start_x, uint8_t start_y, const char* data)
{
  while (*data != '\0')
  {
      OLED_ShowAsciiChar(start_x, start_y, *data);
      start_x += 8;
      CHECK_PAGE_AND_GO_NEXT(start_x, start_y);
      ++data;
  }
}

static void OLED_ShowChineseImpl(uint8_t start_x, uint8_t start_y, const uint8_t* data, uint8_t size)
{
  uint8_t i, j, x, y;
  uint8_t n = size / 16;
  // 显示汉字上半部分
  {
    x = start_x; y = start_y;
    for (i = 0; i < n; i += 2)
    {
      OLED_SetPos(x, y);
      for (j = 0; j < 16; ++j)
      {
        OLED_WriteChar(data[16*i + j]);
      }
      x += 16;
    }
  }
  // 显示汉字下半部分
  {
    x = start_x; y = start_y + 1;
    for (i = 1; i < n; i += 2)
    {
      OLED_SetPos(x, y);
      for (j = 0; j < 16; ++j)
      {
        OLED_WriteChar(data[16*i + j]);
      }
      x += 16;
    }
  }
}

#define OLED_ShowChinese(x, y, data) OLED_ShowChineseImpl(x, y, data, sizeof(data)/sizeof(data[0]))
#define OLED_ShowImage(x, y, data) OLED_ShowChinese(x, y, data)

void OLED_ShowStartup()
{
  OLED_ShowChinese(0, 0, STARTUP_CHINESE);
}

void OLED_ShowShutdown()
{
  OLED_ShowChinese(0, 0, SHUTDOWN_CHINESE);
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
  OLED_ShowAsciiString(OLED_CENTERED_POS(0, 10), 1, date_str);
  // 显示星期：星期X
  uint8_t week_start_pos = OLED_CENTERED_POS(3, 0);
  OLED_ShowChinese(week_start_pos, 3, WEEK_CHINESE);
  OLED_ShowChinese(week_start_pos+2*16, 3, WEEK_DAY_CHINESE[date.WeekDay-1]);
  // 显示时间：时:分:秒
  char time_str[16];
  snprintf(time_str, sizeof(time_str), "%02d:%02d:%02d", time.Hours, time.Minutes, time.Seconds);
  OLED_ShowAsciiString(OLED_CENTERED_POS(0, 8), 5, time_str);
}

static void OLED_ShowTimeOnTop()
{
  RTC_TimeTypeDef time;
  HAL_RTC_GetTime(&hrtc, &time, RTC_FORMAT_BIN);
  char time_str[8];
  snprintf(time_str, sizeof(time_str), "%02d:%02d", time.Hours, time.Minutes);
  OLED_ShowAsciiString(0, 0, time_str);
}

void OLED_ShowHealthy(int32_t heart_rate, int32_t spo2)
{
  OLED_ShowTimeOnTop();
  char hr_str[6];
  snprintf(hr_str, sizeof(hr_str), "% 3ld", heart_rate);
  char spo2_str[6];
  snprintf(spo2_str, sizeof(spo2_str), "% 3ld%%", spo2);
  // 显示心率
  {
    uint8_t hr_start_pos = OLED_CENTERED_POS(1+4, 4);
    OLED_ShowImage(hr_start_pos, 3, HEART_RATE_IMAGE);
    OLED_ShowAsciiChar(hr_start_pos+16+16, 3, ' ');
    OLED_ShowAsciiString(hr_start_pos+16, 3, hr_str);
    OLED_ShowChinese(hr_start_pos+16+16+16, 3, HEALTHY_CHINESE[0]);
  }
  // 显示血氧
  {
    uint8_t spo2_start_pos = OLED_CENTERED_POS(2, 5);
    OLED_ShowChinese(spo2_start_pos, 6, HEALTHY_CHINESE[1]);
    OLED_ShowAsciiString(spo2_start_pos+2*16, 6, spo2_str);
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
    OLED_ShowChinese(sc_start_pos, 3, STEP_CNT_CHINESE);
    OLED_ShowAsciiString(sc_start_pos+4*16, 3, step_str);
  }
  // 显示卡路里：xxx大卡
  {
    uint8_t calories_start_pos = OLED_CENTERED_POS(2, 4);
    OLED_ShowAsciiString(calories_start_pos, 6, calories_str);
    OLED_ShowChinese(calories_start_pos+4*8, 6, CALORIES_UNIT_CHINESE);
  }
}

void OLED_ShowComingCall(const char* phone_number, bool isAcceptCall)
{
  OLED_ShowChinese(OLED_CENTERED_POS(2, 0), 0, COMING_CALL_CHINESE); 
  OLED_ShowAsciiString(OLED_CENTERED_POS(0, 11), 2, phone_number);
  OLED_ClearTargetRow(6);
  if (isAcceptCall)
  {
    OLED_ShowChinese(0, 6, CALL_ACCEPT_CHINESE);
    OLED_ShowChinese(2*16, 6, ACCEPT_SYMBOL);
    OLED_ShowChinese(6*16, 6, CALL_REJECT_CHINESE);
  }
  else
  {
    OLED_ShowChinese(0, 6, CALL_ACCEPT_CHINESE);
    OLED_ShowChinese(5*16, 6, CALL_REJECT_CHINESE);
    OLED_ShowChinese(7*16, 6, ACCEPT_SYMBOL);
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
