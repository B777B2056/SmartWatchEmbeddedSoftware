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

extern user_key_t confirm_key_obj;
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

// static void OLED_ShowShutdown()
// {
//   OLED_Driver_ShowChinese(0, 0, SHUTDOWN_CHINESE);
// }

static void OLED_ShowComponetOnTop()
{
  // 时间
  RTC_TimeTypeDef time;
  HAL_RTC_GetTime(&hrtc, &time, RTC_FORMAT_BIN);
  char time_str[8];
  snprintf(time_str, sizeof(time_str), "%02d:%02d", time.Hours, time.Minutes);
  OLED_Driver_ShowAsciiString(0, 0, time_str);
  // 未接来电
  if (0 != coming_call_handler_obj.missed_call_count)
  {
    OLED_Driver_ShowImage(OLED_WIDTH-16-16, 0, MISSED_CALL_IMAGE);
    char missed_call_cnt_str[4];
    snprintf(missed_call_cnt_str, sizeof(missed_call_cnt_str), "%2hu", coming_call_handler_obj.missed_call_count);
    OLED_Driver_ShowAsciiString(OLED_WIDTH-16, 0, missed_call_cnt_str);
  }
  // 蓝牙连接状态
  // 电池剩余电量
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

static void _ScreenStopwatch_GetTimeStrFromCounter(screen_stopwatch_t* this)
{
  snprintf(this->time_cnt_str, sizeof(this->time_cnt_str), "%02lu:%02lu:%02lu", this->time_count/6000, (this->time_count%6000)/100, this->time_count%100);
}

static void _ScreenStopwatch_RefreshShow(screen_abstract_t* base)
{
  screen_stopwatch_t* this = (screen_stopwatch_t*)base;
  _ScreenStopwatch_GetTimeStrFromCounter(this);
  OLED_ShowComponetOnTop();
  OLED_Driver_ShowAsciiString(OLED_CENTERED_POS(0, 8), 3, this->time_cnt_str);
}

static void _ScreenStopwatch_FirstShow(screen_abstract_t* base)
{
  screen_stopwatch_t* this = (screen_stopwatch_t*)base;
  this->time_count = 0;
  _ScreenStopwatch_RefreshShow(base);
}

void ScreenStopwatch_Ctor(screen_stopwatch_t* this)
{
  DERIVED_CLASS_INIT(this, _ScreenStopwatch_FirstShow, _ScreenStopwatch_RefreshShow);
  this->time_count = 0;
  this->is_started = false;
  memset(&this->time_cnt_str, 0, sizeof(this->time_cnt_str));
}

void ScreenStopwatch_Start(screen_stopwatch_t* this)
{
  this->is_started = true;
}

void ScreenStopwatch_Stop(screen_stopwatch_t* this)
{
  this->is_started = false;
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

static void _ScreenPedometer_GetData(screen_pedometer_t* this)
{
  uint16_t step = ADXL345_GetSteps(&adxl345_obj);
  snprintf(this->step_str, sizeof(this->step_str), "%5hu", step);
  snprintf(this->calories_str, sizeof(this->calories_str), "%4d", ADXL345_GetCalories(&adxl345_obj));
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
}

static void _ScreenComingcall_RefreshShow(screen_abstract_t* parent)
{
  screen_coming_call_t* this = (screen_coming_call_t*)parent;
  _ScreenComingcall_ShowVariableInfo(this);
}

void ScreenComingcall_Ctor(screen_coming_call_t* this)
{
  DERIVED_CLASS_INIT(this, _ScreenComingcall_FirstShow, _ScreenComingcall_RefreshShow);

  this->peer_name = NULL;
  this->is_accept_call = true;
}

static void _ScreenIcon_ShowBasicInfo()
{
  OLED_Driver_ShowChinese(OLED_CENTERED_POS(2, 0), 2, STOPWATCH_ICON); 
  OLED_Driver_ShowChinese(OLED_CENTERED_POS(4, 0), 4, HEALTHY_ICON);
  OLED_Driver_ShowChinese(OLED_CENTERED_POS(4, 0), 6, PEDOMETER_ICON);
}

static void _ScreenIcon_ShowVariableInfo(screen_icon_t* this)
{
  switch (this->item)
  {
  case eSTOPWATCH:
    OLED_Driver_ShowAsciiString(0, 2, "->");
    OLED_Driver_ShowAsciiString(0, 4, "  ");
    OLED_Driver_ShowAsciiString(0, 6, "  ");
    break;

  case eHEALTHY:
    OLED_Driver_ShowAsciiString(0, 2, "  ");
    OLED_Driver_ShowAsciiString(0, 4, "->");
    OLED_Driver_ShowAsciiString(0, 6, "  ");
    break;

  case eSPORT:
    OLED_Driver_ShowAsciiString(0, 2, "  ");
    OLED_Driver_ShowAsciiString(0, 4, "  ");
    OLED_Driver_ShowAsciiString(0, 6, "->");
    break;
  
  default:
    break;
  }
}

static void _ScreenIcon_RefreshShow(screen_abstract_t* parent)
{
  screen_icon_t* this = (screen_icon_t*)parent;
  OLED_ShowComponetOnTop();
  _ScreenIcon_ShowVariableInfo(this);
}

static void _ScreenIcon_FirstShow(screen_abstract_t* parent)
{
  screen_icon_t* this = (screen_icon_t*)parent;
  OLED_ShowComponetOnTop();
  _ScreenIcon_ShowBasicInfo();
  _ScreenIcon_ShowVariableInfo(this);
}

void ScreenIcon_Ctor(screen_icon_t* this)
{
  DERIVED_CLASS_INIT(this, _ScreenIcon_FirstShow, _ScreenIcon_RefreshShow);
  this->item= eSTOPWATCH;
}

void ScreenManager_Ctor(screen_manager_t* this)
{
  this->page_idx = 0;
  this->cur_page = NULL;
  this->is_in_coming_call = false;
  OLED_Driver_Init(&hi2c1);
  ScreenDate_Ctor(&this->date_page);
  ScreenIcon_Ctor(&this->icon_page);
  ScreenStopwatch_Ctor(&this->stopwatch_page);
  ScreenHealthy_Ctor(&this->healthy_page);
  ScreenPedometer_Ctor(&this->pedometer_page);
  ScreenComingcall_Ctor(&this->coming_call_page);
  OLED_DisplayOn();
  OLED_Driver_SetPos(0, 0);
  Screen_Clear();
  ScreenManager_ShowStart(this);
}

void ScreenManager_ShowStart(screen_manager_t* this)
{
  OLED_ShowStartup();
}

void ScreenManager_ShowStop(screen_manager_t* this)
{
  Screen_Clear();
  OLED_DisplayOff();
}

bool ScreenManager_IsInStopwatch(screen_manager_t* this)
{
  return this->cur_page == (screen_abstract_t*)&this->stopwatch_page;
}

static void ScreenManager_ShowStopwatchIcon(screen_manager_t* this)
{
  this->icon_page.item = eSTOPWATCH;
  this->icon_page.parent.is_showed_before = (this->cur_page == (screen_abstract_t*)&this->icon_page);
  ScreenAbstract_Show((screen_abstract_t*)&this->icon_page);
  this->cur_page = (screen_abstract_t*)&this->icon_page;
}

static void ScreenManager_ShowHealthyIcon(screen_manager_t* this)
{
  this->icon_page.item = eHEALTHY;
  this->icon_page.parent.is_showed_before = (this->cur_page == (screen_abstract_t*)&this->icon_page);
  ScreenAbstract_Show((screen_abstract_t*)&this->icon_page);
  this->cur_page = (screen_abstract_t*)&this->icon_page;
}

static void ScreenManager_ShowPedometerIcon(screen_manager_t* this)
{
  this->icon_page.item = eSPORT;
  this->icon_page.parent.is_showed_before = (this->cur_page == (screen_abstract_t*)&this->icon_page);
  ScreenAbstract_Show((screen_abstract_t*)&this->icon_page);
  this->cur_page = (screen_abstract_t*)&this->icon_page;
}

static void ScreenManager_ShowDatePage(screen_manager_t* this)
{
  this->date_page.parent.is_showed_before = (this->cur_page == (screen_abstract_t*)&this->date_page);
  ScreenAbstract_Show((screen_abstract_t*)&this->date_page);
  this->cur_page = (screen_abstract_t*)&this->date_page;
}

static void ScreenManager_ShowStopwatchPage(screen_manager_t* this)
{
  this->stopwatch_page.parent.is_showed_before = (this->cur_page == (screen_abstract_t*)&this->stopwatch_page);
  ScreenAbstract_Show((screen_abstract_t*)&this->stopwatch_page);
  this->cur_page = (screen_abstract_t*)&this->stopwatch_page;
}

static void ScreenManager_ShowHealthyPage(screen_manager_t* this)
{
  this->healthy_page.parent.is_showed_before = (this->cur_page == (screen_abstract_t*)&this->healthy_page);
  ScreenAbstract_Show((screen_abstract_t*)&this->healthy_page);
  this->cur_page = (screen_abstract_t*)&this->healthy_page;
}

static void ScreenManager_ShowPedometerPage(screen_manager_t* this)
{
  this->pedometer_page.parent.is_showed_before = (this->cur_page == (screen_abstract_t*)&this->pedometer_page);
  ScreenAbstract_Show((screen_abstract_t*)&this->pedometer_page);
  this->cur_page = (screen_abstract_t*)&this->pedometer_page;
}

const menu_jump_table_t menu_jump_table[7] =
{
  // 第0层
  {0,0,1,ScreenManager_ShowDatePage},
  // 第1层
  {1,2,4,ScreenManager_ShowStopwatchIcon},
  {2,3,5,ScreenManager_ShowHealthyIcon},
  {3,0,6,ScreenManager_ShowPedometerIcon},
  // 第2层
  {4,4,1,ScreenManager_ShowStopwatchPage},
  {5,5,2,ScreenManager_ShowHealthyPage},
  {6,6,3,ScreenManager_ShowPedometerPage},                  
};

static void ScreenManager_GoComingCallPage(screen_manager_t* this)
{
  this->coming_call_page.parent.is_showed_before = (this->cur_page == (screen_abstract_t*)&this->coming_call_page);
  ScreenAbstract_Show((screen_abstract_t*)&this->coming_call_page);
  this->cur_page = (screen_abstract_t*)&this->coming_call_page;
}

void ScreenManager_PageSwitch(screen_manager_t* this)
{
  this->page_idx = menu_jump_table[this->page_idx].down;
}

void ScreenManager_IntoNextLevelMenu(screen_manager_t* this)
{
  this->page_idx = menu_jump_table[this->page_idx].enter;
}

void ScreenManager_Schedule(screen_manager_t* this)
{
  if (this->is_in_coming_call)
    ScreenManager_GoComingCallPage(this);
  else
    menu_jump_table[this->page_idx].operation(this);
}
