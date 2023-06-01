#include "oled.h"
#include "cmsis_os.h"
#include "i2c.h"
#include "rtc.h"
#include <stdio.h>
#include "oledfont.h"

#define OLED_ADDR 0x78

#define OLED_WIDTH 128
#define OLED_HEIGHT 32

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
  HAL_I2C_Mem_Write(&hi2c2, OLED_ADDR, 0x00, I2C_MEMADD_SIZE_8BIT, &cmd, 1, 0x100);
}

static void OLED_WriteChar(uint8_t data)
{
  HAL_I2C_Mem_Write(&hi2c2, OLED_ADDR, 0x40, I2C_MEMADD_SIZE_8BIT, &data, 1, 0x100);
}

static void OLED_SetPos(uint8_t x, uint8_t y)
{
	OLED_WriteCmd(0xb0 + y);
	OLED_WriteCmd(((x & 0xf0) >> 4) | 0x10);
	OLED_WriteCmd((x & 0x0f) | 0x00);
}

void OLED_Init()
{
  osDelay(100);
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
  uint8_t i, n;
	for (i = 0; i < 4; i++)
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
  uint8_t n = size / 32;
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
      x += j;
      CHECK_PAGE_AND_GO_NEXT(x, y);
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
      x += j;
      CHECK_PAGE_AND_GO_NEXT(x, y);
    }
  }
}

#define OLED_ShowChinese(x, y, data) OLED_ShowChineseImpl(x, y, data, sizeof(data)/sizeof(data[0]))

void OLED_ShowStartup()
{
  OLED_Clear();
  OLED_ShowChinese(0, 0, STARTUP_CHINESE);
}

void OLED_ShowShutdown()
{
  OLED_Clear();
  OLED_ShowChinese(0, 0, SHUTDOWN_CHINESE);
}

void OLED_ShowDate()
{
  // 获取时间
  RTC_DateTypeDef date;
  RTC_TimeTypeDef time;
  HAL_RTC_GetDate(&hrtc, &date, RTC_FORMAT_BIN);
  HAL_RTC_GetTime(&hrtc, &time, RTC_FORMAT_BIN);
  // 清屏
  OLED_Clear();
  // 显示日期：年-月-日
  char date_str[32];
  snprintf(date_str, sizeof(date_str), "20%2hhu-%2hhu-%2hhu", date.Year, date.Month, date.Date);
  OLED_ShowAsciiString(0, 0, date_str);
  // 显示星期：星期X
  OLED_ShowChinese(0, 2, WEEK_CHINESE);
  OLED_ShowChinese(2*16, 2, WEEK_DAY_CHINESE[date.WeekDay-1]);
  // 显示时间：时:分:秒
  char time_str[32];
  snprintf(time_str, sizeof(time_str), "%2hhu:%2hhu:%2hhu", time.Hours, time.Minutes, time.Seconds);
  OLED_ShowAsciiString(0, 4, time_str);
}

void OLED_ShowHealthy(int32_t heart_rate, int32_t spo2)
{
  char hr_str[5];
  snprintf(hr_str, sizeof(hr_str), "%ld", heart_rate);
  char spo2_str[5];
  snprintf(spo2_str, sizeof(spo2_str), "%ld%%", spo2);
  OLED_Clear();
  // 显示心率：心率xx
  OLED_ShowChinese(0, 0, HEALTHY_CHINESE[0]);
  OLED_ShowAsciiString(2*16, 0, hr_str);
  // 显示血氧：血氧xx%
  OLED_ShowChinese(0, 2, HEALTHY_CHINESE[1]);
  OLED_ShowAsciiString(2*16, 2, spo2_str);
}

void OLED_ShowStepCnt(uint16_t step)
{
  char step_str[6];
  snprintf(step_str, sizeof(step_str), "%d", step);
  OLED_Clear();
  // 显示计步：今日步数xxxx
  OLED_ShowChinese(0, 0, STEP_CNT_CHINESE);
  OLED_ShowAsciiString(4*16, 0, step_str);
}

void OLED_ShowComingCall(const char* phone_number, bool isAcceptCall)
{
  OLED_Clear();
  OLED_ShowChinese(0, 0, COMING_CALL_CHINESE);
  OLED_ShowAsciiString(2*16, 0, phone_number);
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
  osMutexWait(pageSwitchMutexHandle, osWaitForever);
  current_page = (current_page+1) % (PAGE_N-1); // Coming call page not show in normal
  osMutexRelease(pageSwitchMutexHandle);
}
