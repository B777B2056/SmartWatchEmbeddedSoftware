#ifndef __OLED_H__
#define __OLED_H__
#include "rtc.h"
#include <stdint.h>
#include <stdbool.h>

#define FUNCTIONAL_PAGE_N 3

void Screen_Clear();

struct SCREEN_VTABLE;

typedef struct
{
  struct SCREEN_VTABLE* vptr; // ptr to virtual table

  bool is_showed_before;
} screen_abstract_t;

void ScreenAbstract_Ctor(screen_abstract_t* this);
void ScreenAbstract_Show(screen_abstract_t* this);
void ScreenAbstract_FirstShow(screen_abstract_t* this);
void ScreenAbstract_RefreshShow(screen_abstract_t* this);

/* Virtual tabel */
typedef struct SCREEN_VTABLE
{
  void (*first_show)(screen_abstract_t* this); // first show page on screen
  void (*refresh_show)(screen_abstract_t* this); // show page on screen
} screen_vtabel;

typedef struct
{
  screen_abstract_t parent;

  char date_str[32];
  char time_str[16];
  RTC_DateTypeDef date;
  RTC_TimeTypeDef time;
} screen_date_t;

void ScreenDate_Ctor(screen_date_t* this);

typedef struct
{
  screen_abstract_t parent;

  int32_t heart_rate;
  int32_t spo2;
  char hr_str[10];
  char spo2_str[6];
} screen_healthy_t;

void ScreenHealthy_Ctor(screen_healthy_t* this);

typedef struct
{
  screen_abstract_t parent;

  char step_str[6];
  char calories_str[8];
} screen_pedometer_t;

void ScreenPedometer_Ctor(screen_pedometer_t* this);

typedef struct
{
  screen_abstract_t parent;

  char* peer_name;
  bool is_accept_call;
} screen_coming_call_t;

void ScreenComingcall_Ctor(screen_coming_call_t* this);

typedef struct
{
  screen_date_t date_page;
  screen_healthy_t healthy_page;
  screen_pedometer_t pedometer_page;
  screen_coming_call_t coming_call_page;

  screen_abstract_t* pages[FUNCTIONAL_PAGE_N+1];
  uint8_t current_page_idx;
  screen_abstract_t* current_page;
  bool is_in_coming_call;
} screen_manager_t;

void ScreenManager_Ctor(screen_manager_t* this);
void ScreenManager_ShowStart(screen_manager_t* this);
void ScreenManager_ShowStop(screen_manager_t* this);
void ScreenManager_ShowCurrentPage(screen_manager_t* this);
void ScreenManager_GoNextPage(screen_manager_t* this);
void ScreenManager_GoComingCallPage(screen_manager_t* this);
void ScreenManager_RecoverFromComingCall(screen_manager_t* this);

#endif
