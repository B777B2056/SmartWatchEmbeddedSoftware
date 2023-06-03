#ifndef __OLED_H__
#define __OLED_H__
#include <stdint.h>
#include <stdbool.h>

typedef uint8_t PageType;

#define DATE_PAGE 0
#define HEALTHY_PAGE 1
#define STEP_CNT_PAGE 2
#define COMING_CALL_PAGE 3
#define PAGE_N 4

typedef struct
{
  PageType current_page;
} oled_t;

void OLED_Init(oled_t* obj);                                                             // OLED初始化
void OLED_Clear();                                                            // OLED清屏
void OLED_DisplayOn();                                                        // OLED开启
void OLED_DisplayOff();                                                       // OLED关闭
void OLED_ShowStartup();                                                      // 开机界面：显示开机画面
void OLED_ShowShutdown();                                                     // 关机界面：显示关机画面
void OLED_ShowDate();                                                         // 主界面：显示当前日期和时间
void OLED_ShowHealthy(int32_t heart_rate, int32_t spo2);                    // 健康界面：显示当前心率和血氧
void OLED_ShowStepCnt(uint16_t step);                                       // 计步界面：显示今日当前步数
void OLED_ShowComingCall(const char* phone_number, bool isAcceptCall);      // 来电通知：显示当前来电号码，与接听选项（接听/拒接）
void OLED_SetCurrentPage(oled_t* obj, PageType page);
PageType OLED_CurrentPage(oled_t* obj);
void OLED_GoNextPage(oled_t* obj);

#endif
