#include "oled.h"
#include "i2c.h"
#include "gpio.h"
#include <assert.h>
#include <stdio.h>
#include <string.h>
#include "hc06.h"
#include "max30102.h"
#include "adxl345.h"
#include "oled_hal.h"
#include "oledfont.h"
#include "oledimg.h"

#define ASCII_SYMBOL_WIDTH 8
#define CHINESE_SYMBOL_WIDTH 16
#define OLED_CENTERED_POS(CHINESE_N, ASCII_N) \
  (((OLED_WIDTH)-((ASCII_SYMBOL_WIDTH)*(ASCII_N))-((CHINESE_SYMBOL_WIDTH)*(CHINESE_N)))/2)

#define DERIVED_CLASS_INIT(obj, first_show_impl, refresh_show_impl) \
  do  \
  { \
    ScreenAbstract_Ctor(&obj->parent); \
    static screen_vtabel vtbl = \
    { \
      .first_show = first_show_impl,  \
      .refresh_show = refresh_show_impl,  \
    };  \
    obj->parent.vptr = &vtbl;  \
  } while (0)

extern max30102_t max30102_obj;
extern adxl345_t adxl345_obj;
extern coming_call_handler_t coming_call_handler_obj;

void Screen_Clear()
{
  OLED_Driver_ClearTargetRowBelow(0);
}

static void OLED_DisplayOn()
{
	OLED_Driver_WriteCmd(0x8D);
	OLED_Driver_WriteCmd(0x14);
	OLED_Driver_WriteCmd(0xAF);
}

static void OLED_DisplayOff()
{
	OLED_Driver_WriteCmd(0x8D);
	OLED_Driver_WriteCmd(0x10);
	OLED_Driver_WriteCmd(0xAF);
}

static void OLED_ShowStartup()
{
  OLED_Driver_ShowChinese(0, 0, STARTUP_CHINESE);
}

static void OLED_ShowShutdown()
{
  OLED_Driver_ShowChinese(0, 0, SHUTDOWN_CHINESE);
}

static void OLED_ShowComponetOnTop()
{
  RTC_TimeTypeDef time;
  HAL_RTC_GetTime(&hrtc, &time, RTC_FORMAT_BIN);
  char time_str[8];
  snprintf(time_str, sizeof(time_str), "%02d:%02d", time.Hours, time.Minutes);
  OLED_Driver_ShowAsciiString(0, 0, time_str);
  if (0 != coming_call_handler_obj.missed_call_count)
  {
    OLED_Driver_ShowImage(OLED_WIDTH-16-16, 0, MISSED_CALL_IMAGE);
    char missed_call_cnt_str[4];
    snprintf(missed_call_cnt_str, sizeof(missed_call_cnt_str), "%2hu", coming_call_handler_obj.missed_call_count);
    OLED_Driver_ShowAsciiString(OLED_WIDTH-16, 0, missed_call_cnt_str);
  }
}

static void _ScreenAbstract_FirstShow(screen_abstract_t* this)
{
  assert(0);
}

static void _ScreenAbstract_RefreshShow(screen_abstract_t* this)
{
  assert(0);
}

void ScreenAbstract_Ctor(screen_abstract_t* this)
{
  static screen_vtabel vtbl =
  {
    .first_show = _ScreenAbstract_FirstShow,
    .refresh_show = _ScreenAbstract_RefreshShow,
  };
  this->vptr = &vtbl;
  this->is_showed_before = false;
}

void ScreenAbstract_Show(screen_abstract_t* this)
{
  if (!this->is_showed_before)
  {
    Screen_Clear();
    ScreenAbstract_FirstShow(this);
    this->is_showed_before = true;
  }
  else
  {
    ScreenAbstract_RefreshShow(this);
  }
}

void ScreenAbstract_FirstShow(screen_abstract_t* this)
{
  this->vptr->first_show(this);
}

void ScreenAbstract_RefreshShow(screen_abstract_t* this)
{
  this->vptr->refresh_show(this);
}

static void _ScreenDate_RefreshShow(screen_abstract_t* parent)
{
  screen_date_t* this = (screen_date_t*)parent;
  // 获取时间
  HAL_RTC_GetDate(&hrtc, &this->date, RTC_FORMAT_BIN);
  HAL_RTC_GetTime(&hrtc, &this->time, RTC_FORMAT_BIN);
  // 显示日期：年-月-日
  snprintf(this->date_str, sizeof(this->date_str), "20%02d-%02d-%02d", this->date.Year, this->date.Month, this->date.Date);
  OLED_Driver_ShowAsciiString(OLED_CENTERED_POS(0, 10), 1, this->date_str);
  // 显示星期：星期X
  uint8_t week_start_pos = OLED_CENTERED_POS(3, 0);
  OLED_Driver_ShowChinese(week_start_pos, 3, WEEK_CHINESE);
  OLED_Driver_ShowChinese(week_start_pos+2*16, 3, WEEK_DAY_CHINESE[this->date.WeekDay-1]);
  // 显示时间：时:分:秒
  snprintf(this->time_str, sizeof(this->time_str), "%02d:%02d:%02d", this->time.Hours, this->time.Minutes, this->time.Seconds);
  OLED_Driver_ShowAsciiString(OLED_CENTERED_POS(0, 8), 5, this->time_str);
}

static void _ScreenDate_FirstShow(screen_abstract_t* parent)
{
  _ScreenDate_RefreshShow(parent);
}

void ScreenDate_Ctor(screen_date_t* this)
{
  DERIVED_CLASS_INIT(this, _ScreenDate_FirstShow, _ScreenDate_RefreshShow);

  memset(&this->date, 0, sizeof(this->date));
  memset(&this->time, 0, sizeof(this->time));
}

static void _ScreenHealthy_AccquireHealthyData(screen_healthy_t* this)
{
  // MAX30102_DoSample(&max30102_obj);
  MAX30102_GetData(&max30102_obj, &this->heart_rate, &this->spo2);
  snprintf(this->hr_str, sizeof(this->hr_str), "%3ld", this->heart_rate);
  snprintf(this->spo2_str, sizeof(this->spo2_str), "%3ld%%", this->spo2);
}

static void _ScreenHealthy_RefreshShow(screen_abstract_t* parent)
{
  screen_healthy_t* this = (screen_healthy_t*)parent;
  OLED_ShowComponetOnTop();
  _ScreenHealthy_AccquireHealthyData(this);

  OLED_Driver_ShowAsciiString(OLED_CENTERED_POS(3, 5)+16+8, 3, this->hr_str);
  OLED_Driver_ShowAsciiString(OLED_CENTERED_POS(2, 5)+2*16, 6, this->spo2_str);
}

static void _ScreenHealthy_FirstShow(screen_abstract_t* parent)
{
  screen_healthy_t* this = (screen_healthy_t*)parent;
  OLED_ShowComponetOnTop();
  _ScreenHealthy_AccquireHealthyData(this);
  // 显示心率
  {
    uint8_t hr_start_pos = OLED_CENTERED_POS(3, 5);
    OLED_Driver_ShowImage(hr_start_pos, 3, HEART_RATE_IMAGE);
    OLED_Driver_ShowAsciiString(hr_start_pos+16+8, 3, this->hr_str);
    OLED_Driver_ShowAsciiString(hr_start_pos+16+8*5, 3, "BPM");
  }
  // 显示血氧
  {
    uint8_t spo2_start_pos = OLED_CENTERED_POS(2, 5);
    OLED_Driver_ShowChinese(spo2_start_pos, 6, HEALTHY_CHINESE[2]);
    OLED_Driver_ShowAsciiString(spo2_start_pos+2*16, 6, this->spo2_str);
  }
}

void ScreenHealthy_Ctor(screen_healthy_t* this)
{
  DERIVED_CLASS_INIT(this, _ScreenHealthy_FirstShow, _ScreenHealthy_RefreshShow);
  
  this->heart_rate = this->spo2 = 0;
}

static uint16_t CalculateCalories(uint16_t step)
{
  return step * 0.046;
}

static void _ScreenPedometer_GetData(screen_pedometer_t* this)
{
  uint16_t step = ADXL345_GetSteps(&adxl345_obj);
  snprintf(this->step_str, sizeof(this->step_str), "%5hu", step);
  snprintf(this->calories_str, sizeof(this->calories_str), "%4d", CalculateCalories(step));
}

static void _ScreenPedometer_RefreshShow(screen_abstract_t* parent)
{
  screen_pedometer_t* this = (screen_pedometer_t*)parent;
  OLED_ShowComponetOnTop();
  _ScreenPedometer_GetData(this);

  OLED_Driver_ShowAsciiString(OLED_CENTERED_POS(4, 4)+4*16, 3, this->step_str);
  OLED_Driver_ShowAsciiString(OLED_CENTERED_POS(2, 4), 6, this->calories_str);
}

static void _ScreenPedometer_FirstShow(screen_abstract_t* parent)
{
  screen_pedometer_t* this = (screen_pedometer_t*)parent;
  OLED_ShowComponetOnTop();
  _ScreenPedometer_GetData(this);
  // 显示计步：今日步数xxxx
  {
    uint8_t sc_start_pos = OLED_CENTERED_POS(4, 4);
    OLED_Driver_ShowChinese(sc_start_pos, 3, STEP_CNT_CHINESE);
    OLED_Driver_ShowAsciiString(sc_start_pos+4*16, 3, this->step_str);
  }
  // 显示卡路里：xxx大卡
  {
    uint8_t calories_start_pos = OLED_CENTERED_POS(2, 4);
    OLED_Driver_ShowAsciiString(calories_start_pos, 6, this->calories_str);
    OLED_Driver_ShowChinese(calories_start_pos+4*8, 6, CALORIES_UNIT_CHINESE);
  }
}

void ScreenPedometer_Ctor(screen_pedometer_t* this)
{
  DERIVED_CLASS_INIT(this, _ScreenPedometer_FirstShow, _ScreenPedometer_RefreshShow);
}

static void _ScreenComingcall_DetectUserChoiceInComingCall(screen_coming_call_t* this)
{
  if (KEY_ON == KeyScan(CONFIRM_KEY))
  {
    // Notify bluetooth to do work
    ComingCallHandler_SetChoice(&coming_call_handler_obj, this->is_accept_call);
  }
}

static void _ScreenComingcall_ShowBasicInfo()
{
  OLED_Driver_ShowChinese(OLED_CENTERED_POS(2, 0), 0, COMING_CALL_CHINESE); 
  OLED_Driver_ShowChinese(16, 6, CALL_ACCEPT_CHINESE);
  OLED_Driver_ShowChinese(6*16, 6, CALL_REJECT_CHINESE);
}

static void _ScreenComingcall_ShowVariableInfo(screen_coming_call_t* this)
{
  const char* phone_number = ComingCallHandler_GetContent(&coming_call_handler_obj);
  OLED_Driver_ClearTargetRow(3);  OLED_Driver_ClearTargetRow(4);  // flash phone number
  OLED_Driver_ShowAsciiString(OLED_CENTERED_POS(0, strlen(phone_number)), 3, phone_number);
  if (this->is_accept_call)
  {
    OLED_Driver_ShowChinese(0, 6, ACCEPT_SYMBOL);
    OLED_Driver_ShowChinese(5*16, 6, REJECT_SYMBOL);
  }
  else
  {
    OLED_Driver_ShowChinese(5*16, 6, ACCEPT_SYMBOL);
    OLED_Driver_ShowChinese(0, 6, REJECT_SYMBOL);
  }
}

static void _ScreenComingcall_FirstShow(screen_abstract_t* parent)
{
  screen_coming_call_t* this = (screen_coming_call_t*)parent;
  _ScreenComingcall_ShowBasicInfo();
  _ScreenComingcall_ShowVariableInfo(this);
  _ScreenComingcall_DetectUserChoiceInComingCall(this);
}

static void _ScreenComingcall_RefreshShow(screen_abstract_t* parent)
{
  screen_coming_call_t* this = (screen_coming_call_t*)parent;
  _ScreenComingcall_ShowVariableInfo(this);
  _ScreenComingcall_DetectUserChoiceInComingCall(this);
}

void ScreenComingcall_Ctor(screen_coming_call_t* this)
{
  DERIVED_CLASS_INIT(this, _ScreenComingcall_FirstShow, _ScreenComingcall_RefreshShow);

  this->peer_name = NULL;
  this->is_accept_call = true;
}

void ScreenManager_Ctor(screen_manager_t* this)
{
  ScreenDate_Ctor(&this->date_page);
  ScreenHealthy_Ctor(&this->healthy_page);
  ScreenPedometer_Ctor(&this->pedometer_page);
  ScreenComingcall_Ctor(&this->coming_call_page);

  this->pages[0] = (screen_abstract_t*)&this->date_page;
  this->pages[1] = (screen_abstract_t*)&this->pedometer_page;
  this->pages[2] = (screen_abstract_t*)&this->healthy_page;
  this->pages[3] = (screen_abstract_t*)&this->coming_call_page;
  this->current_page_idx = 0;
  this->current_page = (screen_abstract_t*)&this->date_page;
  this->is_in_coming_call = false;

  OLED_Driver_Init(&hi2c1);
  Screen_Clear();
  OLED_Driver_SetPos(0, 0);
  OLED_DisplayOn();
  OLED_ShowStartup();
}

void ScreenManager_Dtor(screen_manager_t* this)
{
  OLED_ShowShutdown();
  OLED_DisplayOff();
}

void ScreenManager_ShowCurrentPage(screen_manager_t* this)
{
  ScreenAbstract_Show(this->current_page);
  this->is_in_coming_call = (this->current_page == (screen_abstract_t*)&this->coming_call_page);
}

void ScreenManager_GoNextPage(screen_manager_t* this)
{
  this->current_page_idx = (this->current_page_idx+1)%FUNCTIONAL_PAGE_N;
  this->current_page->is_showed_before = false;
  this->current_page = this->pages[this->current_page_idx];
}

void ScreenManager_GoComingCallPage(screen_manager_t* this)
{
  this->current_page->is_showed_before = false;
  this->current_page = (screen_abstract_t*)&this->coming_call_page;
}

void ScreenManager_RecoverFromComingCall(screen_manager_t* this)
{
  this->current_page->is_showed_before = false;
  this->current_page = this->pages[this->current_page_idx];
}
