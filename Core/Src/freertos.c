/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * File Name          : freertos.c
  * Description        : Code for freertos applications
  ******************************************************************************
  * @attention
  *
  * Copyright (c) 2023 STMicroelectronics.
  * All rights reserved.
  *
  * This software is licensed under terms that can be found in the LICENSE file
  * in the root directory of this software component.
  * If no LICENSE file comes with this software, it is provided AS-IS.
  *
  ******************************************************************************
  */
/* USER CODE END Header */

/* Includes ------------------------------------------------------------------*/
#include "FreeRTOS.h"
#include "task.h"
#include "main.h"
#include "cmsis_os.h"

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */
#include "oled.h"
#include "hc06.h"
#include "max30102.h"
#include "adxl345.h"
#include "gpio.h"
#include <stdbool.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
/* USER CODE END Includes */

/* Private typedef -----------------------------------------------------------*/
/* USER CODE BEGIN PTD */

/* USER CODE END PTD */

/* Private define ------------------------------------------------------------*/
/* USER CODE BEGIN PD */

/* USER CODE END PD */

/* Private macro -------------------------------------------------------------*/
/* USER CODE BEGIN PM */
#define SEND_DATA_TO_BLUETOOTH(hc06_obj_ptr, msg_type, data) \
  do  \
  { \
    char msg[16]; \
    msg[0] = msg_type;  \
    msg[1] = snprintf(msg+2, sizeof(msg)-2, "%d", (int)data); \
    HC06_SendMsg(hc06_obj_ptr, msg, 2+msg[1]);  \
  } while (0)
  
/* USER CODE END PM */

/* Private variables ---------------------------------------------------------*/
/* USER CODE BEGIN Variables */
bool is_sys_shutdown;
user_key_t page_key_obj, confirm_key_obj;
adxl345_t adxl345_obj;
screen_manager_t screen_obj;
max30102_t max30102_obj;
hc06_t hc06_obj;
coming_call_handler_t coming_call_handler_obj;
/* USER CODE END Variables */
osThreadId frontendTaskHandle;
osThreadId backendTaskHandle;
osTimerId keyScanTimerHandle;
osTimerId stopwatchTimerHandle;
osTimerId dataSendTimerHandle;
osMutexId stepCntGetterMutexHandle;
osMutexId healthyGetterMutexHandle;

/* Private function prototypes -----------------------------------------------*/
/* USER CODE BEGIN FunctionPrototypes */
/* USER CODE END FunctionPrototypes */

void FrontendRun(void const * argument);
void BackendRun(void const * argument);
void KeyScanTimerCallback(void const * argument);
void StopwatchCallback(void const * argument);
void DataSendCallback(void const * argument);

void MX_FREERTOS_Init(void); /* (MISRA C 2004 rule 8.1) */

/* GetIdleTaskMemory prototype (linked to static allocation support) */
void vApplicationGetIdleTaskMemory( StaticTask_t **ppxIdleTaskTCBBuffer, StackType_t **ppxIdleTaskStackBuffer, uint32_t *pulIdleTaskStackSize );

/* GetTimerTaskMemory prototype (linked to static allocation support) */
void vApplicationGetTimerTaskMemory( StaticTask_t **ppxTimerTaskTCBBuffer, StackType_t **ppxTimerTaskStackBuffer, uint32_t *pulTimerTaskStackSize );

/* USER CODE BEGIN GET_IDLE_TASK_MEMORY */
static StaticTask_t xIdleTaskTCBBuffer;
static StackType_t xIdleStack[configMINIMAL_STACK_SIZE];

void vApplicationGetIdleTaskMemory( StaticTask_t **ppxIdleTaskTCBBuffer, StackType_t **ppxIdleTaskStackBuffer, uint32_t *pulIdleTaskStackSize )
{
  *ppxIdleTaskTCBBuffer = &xIdleTaskTCBBuffer;
  *ppxIdleTaskStackBuffer = &xIdleStack[0];
  *pulIdleTaskStackSize = configMINIMAL_STACK_SIZE;
  /* place for user code */
}
/* USER CODE END GET_IDLE_TASK_MEMORY */

/* USER CODE BEGIN GET_TIMER_TASK_MEMORY */
static StaticTask_t xTimerTaskTCBBuffer;
static StackType_t xTimerStack[configTIMER_TASK_STACK_DEPTH];

void vApplicationGetTimerTaskMemory( StaticTask_t **ppxTimerTaskTCBBuffer, StackType_t **ppxTimerTaskStackBuffer, uint32_t *pulTimerTaskStackSize )
{
  *ppxTimerTaskTCBBuffer = &xTimerTaskTCBBuffer;
  *ppxTimerTaskStackBuffer = &xTimerStack[0];
  *pulTimerTaskStackSize = configTIMER_TASK_STACK_DEPTH;
  /* place for user code */
}
/* USER CODE END GET_TIMER_TASK_MEMORY */

/**
  * @brief  FreeRTOS initialization
  * @param  None
  * @retval None
  */
void MX_FREERTOS_Init(void) {
  /* USER CODE BEGIN Init */

  /* USER CODE END Init */
  /* Create the mutex(es) */
  /* definition and creation of stepCntGetterMutex */
  osMutexDef(stepCntGetterMutex);
  stepCntGetterMutexHandle = osMutexCreate(osMutex(stepCntGetterMutex));

  /* definition and creation of healthyGetterMutex */
  osMutexDef(healthyGetterMutex);
  healthyGetterMutexHandle = osMutexCreate(osMutex(healthyGetterMutex));

  /* USER CODE BEGIN RTOS_MUTEX */
  /* add mutexes, ... */
  /* USER CODE END RTOS_MUTEX */

  /* USER CODE BEGIN RTOS_SEMAPHORES */
  /* add semaphores, ... */
  /* USER CODE END RTOS_SEMAPHORES */

  /* Create the timer(s) */
  /* definition and creation of keyScanTimer */
  osTimerDef(keyScanTimer, KeyScanTimerCallback);
  keyScanTimerHandle = osTimerCreate(osTimer(keyScanTimer), osTimerPeriodic, NULL);

  /* definition and creation of stopwatchTimer */
  osTimerDef(stopwatchTimer, StopwatchCallback);
  stopwatchTimerHandle = osTimerCreate(osTimer(stopwatchTimer), osTimerPeriodic, NULL);

  /* definition and creation of dataSendTimer */
  osTimerDef(dataSendTimer, DataSendCallback);
  dataSendTimerHandle = osTimerCreate(osTimer(dataSendTimer), osTimerPeriodic, NULL);

  /* USER CODE BEGIN RTOS_TIMERS */
  /* start timers, add new ones, ... */
  /* USER CODE END RTOS_TIMERS */

  /* USER CODE BEGIN RTOS_QUEUES */
  /* add queues, ... */
  /* USER CODE END RTOS_QUEUES */

  /* Create the thread(s) */
  /* definition and creation of frontendTask */
  osThreadDef(frontendTask, FrontendRun, osPriorityNormal, 0, 256);
  frontendTaskHandle = osThreadCreate(osThread(frontendTask), NULL);

  /* definition and creation of backendTask */
  osThreadDef(backendTask, BackendRun, osPriorityNormal, 0, 256);
  backendTaskHandle = osThreadCreate(osThread(backendTask), NULL);

  /* USER CODE BEGIN RTOS_THREADS */
  /* add threads, ... */
  /* USER CODE END RTOS_THREADS */

}

/* USER CODE BEGIN Header_FrontendRun */
/**
  * @brief  Function implementing the frontendTask thread.
  * @param  argument: Not used
  * @retval None
  */
/* USER CODE END Header_FrontendRun */
void FrontendRun(void const * argument)
{
  /* USER CODE BEGIN FrontendRun */
  osDelay(500);
  Screen_Clear();
  osTimerStart(keyScanTimerHandle, KEY_TIMER_PERIOD_MS);
  osTimerStart(stopwatchTimerHandle, 10);
  osTimerStart(dataSendTimerHandle, 1000);
  /* Infinite loop */
  for (;;)
  {
    if (is_sys_shutdown)  continue;
    // Call coming
    if (ComingCallHandler_IsNewCallComing(&coming_call_handler_obj))
    {
      screen_obj.is_in_coming_call = true;
    }
    // Call cancel
    if (ComingCallHandler_IsCallHandled(&coming_call_handler_obj))
    {
      screen_obj.is_in_coming_call = false;
    }
    ScreenManager_Schedule(&screen_obj);
  }
  /* USER CODE END FrontendRun */
}

/* USER CODE BEGIN Header_BackendRun */
/**
* @brief Function implementing the backendTask thread.
* @param argument: Not used
* @retval None
*/
/* USER CODE END Header_BackendRun */
void BackendRun(void const * argument)
{
  /* USER CODE BEGIN BackendRun */
  
  /* Infinite loop */
  for (;;)
  {
    if (is_sys_shutdown)  continue;
    // Heart rate & spO2
    MAX30102_DoSample(&max30102_obj);
    // Count steps
    ADXL345_DoStepCnt(&adxl345_obj);
  }
  /* USER CODE END BackendRun */
}

/* KeyScanTimerCallback function */
void KeyScanTimerCallback(void const * argument)
{
  /* USER CODE BEGIN KeyScanTimerCallback */
  switch (KeyScan(&page_key_obj))
  {
  case KEY_ON:
    if (!screen_obj.is_in_coming_call)
    {
      if (ScreenManager_IsInStopwatch(&screen_obj))
        screen_obj.stopwatch_page.is_started = !screen_obj.stopwatch_page.is_started;
      else
        ScreenManager_PageSwitch(&screen_obj);
    }
    else
      screen_obj.coming_call_page.is_accept_call = !screen_obj.coming_call_page.is_accept_call;
    break;

  case KEY_LONG_ON:
    if (is_sys_shutdown)
    {
      is_sys_shutdown = false;
      ScreenManager_ShowStart(&screen_obj);
      Screen_Clear();
    }
    else
    {
      is_sys_shutdown = true;
      ScreenManager_ShowStop(&screen_obj);
    }
    break;
  
  default:
    break;
  }

  switch (KeyScan(&confirm_key_obj))
  {
  case KEY_ON:
    if (screen_obj.is_in_coming_call)
    {
      screen_obj.is_in_coming_call = false;
      // Notify bluetooth to do work
      ComingCallHandler_SetChoice(&coming_call_handler_obj, screen_obj.coming_call_page.is_accept_call);
    }
    else
    {
      ScreenManager_IntoNextLevelMenu(&screen_obj);
    }
    break;
  
  default:
    break;
  }
  /* USER CODE END KeyScanTimerCallback */
}

/* StopwatchCallback function */
void StopwatchCallback(void const * argument)
{
  /* USER CODE BEGIN StopwatchCallback */
  if (screen_obj.stopwatch_page.is_started)
    ++screen_obj.stopwatch_page.time_count;
  /* USER CODE END StopwatchCallback */
}

/* DataSendCallback function */
void DataSendCallback(void const * argument)
{
  /* USER CODE BEGIN DataSendCallback */
  // Send data to APP by Bluetooth
  SEND_DATA_TO_BLUETOOTH(&hc06_obj, MSG_TYPE_STEPCNT, screen_obj.pedometer_page.step_cnt);
  SEND_DATA_TO_BLUETOOTH(&hc06_obj, MSG_TYPE_KAL, screen_obj.pedometer_page.kal);
  SEND_DATA_TO_BLUETOOTH(&hc06_obj, MSG_TYPE_HEART_RATE, screen_obj.healthy_page.heart_rate);
  SEND_DATA_TO_BLUETOOTH(&hc06_obj, MSG_TYPE_SPO2, screen_obj.healthy_page.spo2);
  /* USER CODE END DataSendCallback */
}

/* Private application code --------------------------------------------------*/
/* USER CODE BEGIN Application */
void Objects_Init()
{
  is_sys_shutdown = false;
  // Init Keys
  KeyInit(&page_key_obj, PAGE_CHOOSE_KEY);
  KeyInit(&confirm_key_obj, CONFIRM_KEY);
  // Init Screen
  ScreenManager_Ctor(&screen_obj);
  // Init MAX30102
  MAX30102_Init(&max30102_obj);
  // Init ADXL345
  ADXL345_Init(&adxl345_obj);
  // Init HC-06
  HC06_Init(&hc06_obj);
  // Init Coming call handler
  ComingCallHandler_Init(&coming_call_handler_obj, &hc06_obj);
}

void HAL_UART_RxCpltCallback(UART_HandleTypeDef *huart)
{
  switch (HC06_HandleMsg(&hc06_obj))
  {
  case MSG_TYPE_COMING_CALL_NOTIFY:

    ComingCallHandler_NewCallNotify(&coming_call_handler_obj);
    break;

  case MSG_TYPE_CALL_HANGUP_NOTIFY:
    ComingCallHandler_PeerHangupNotify(&coming_call_handler_obj);
    break;

  case MSG_TYPE_CALL_ACCEPT_NOTIFY:
    ComingCallHandler_AcceptCallNotify(&coming_call_handler_obj);
    break;
  
  case MSG_TYPE_NOT_COMPLETED:
  default:
    break;
  }
}

#ifdef __GNUC__
#define PUTCHAR_PROTOTYPE int __io_putchar(int ch)
#else
#define PUTCHAR_PROTOTYPE int fputc(int ch, FILE *f)
#endif
// 重定向printf到串口USART2（蓝牙）
PUTCHAR_PROTOTYPE
{
    HC06_SendMsg(&hc06_obj, (char*)&ch, 1);
    return ch;
}
/* USER CODE END Application */

