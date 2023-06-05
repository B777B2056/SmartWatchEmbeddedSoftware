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
/* USER CODE END Includes */

/* Private typedef -----------------------------------------------------------*/
/* USER CODE BEGIN PTD */

/* USER CODE END PTD */

/* Private define ------------------------------------------------------------*/
/* USER CODE BEGIN PD */

/* USER CODE END PD */

/* Private macro -------------------------------------------------------------*/
/* USER CODE BEGIN PM */

/* USER CODE END PM */

/* Private variables ---------------------------------------------------------*/
/* USER CODE BEGIN Variables */
adxl345_t adxl345_obj;
screen_manager_t screen_obj;
max30102_t max30102_obj;
hc06_t hc06_obj;
coming_call_handler_t coming_call_handler_obj;
/* USER CODE END Variables */
osThreadId frontendTaskHandle;
osThreadId backendTaskHandle;
osMutexId stepCntMutexHandle;

/* Private function prototypes -----------------------------------------------*/
/* USER CODE BEGIN FunctionPrototypes */

/* USER CODE END FunctionPrototypes */

void FrontendRun(void const * argument);
void BackendRun(void const * argument);

void MX_FREERTOS_Init(void); /* (MISRA C 2004 rule 8.1) */

/* GetIdleTaskMemory prototype (linked to static allocation support) */
void vApplicationGetIdleTaskMemory( StaticTask_t **ppxIdleTaskTCBBuffer, StackType_t **ppxIdleTaskStackBuffer, uint32_t *pulIdleTaskStackSize );

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

/**
  * @brief  FreeRTOS initialization
  * @param  None
  * @retval None
  */
void MX_FREERTOS_Init(void) {
  /* USER CODE BEGIN Init */

  /* USER CODE END Init */
  /* Create the mutex(es) */
  /* definition and creation of stepCntMutex */
  osMutexDef(stepCntMutex);
  stepCntMutexHandle = osMutexCreate(osMutex(stepCntMutex));

  /* USER CODE BEGIN RTOS_MUTEX */
  /* add mutexes, ... */
  /* USER CODE END RTOS_MUTEX */

  /* USER CODE BEGIN RTOS_SEMAPHORES */
  /* add semaphores, ... */
  /* USER CODE END RTOS_SEMAPHORES */

  /* USER CODE BEGIN RTOS_TIMERS */
  /* start timers, add new ones, ... */
  /* USER CODE END RTOS_TIMERS */

  /* USER CODE BEGIN RTOS_QUEUES */
  /* add queues, ... */
  /* USER CODE END RTOS_QUEUES */

  /* Create the thread(s) */
  /* definition and creation of frontendTask */
  osThreadDef(frontendTask, FrontendRun, osPriorityNormal, 0, 128);
  frontendTaskHandle = osThreadCreate(osThread(frontendTask), NULL);

  /* definition and creation of backendTask */
  osThreadDef(backendTask, BackendRun, osPriorityNormal, 0, 128);
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
#ifdef JR_DEBUG
  int i = 1;
#endif
  osDelay(1000);
  Screen_Clear();
  /* Infinite loop */
  for(;;)
  {
    ScreenManager_ShowCurrentPage(&screen_obj);
    // Call coming
    if (ComingCallHandler_IsNewCallComing(&coming_call_handler_obj))
    {
      ScreenManager_GoComingCallPage(&screen_obj);
    }
    // Call cancel
    if (ComingCallHandler_IsHangupByPeer(&coming_call_handler_obj))
    {
      ScreenManager_RecoverFromComingCall(&screen_obj);
    }
    // Detect and switch to next page
#ifndef JR_DEBUG
    if (KEY_ON == KeyScan(PAGE_CHOOSE_KEY))
#else
    if (i%100 == 0)
#endif
    {
      if (!screen_obj.is_in_coming_call)
        ScreenManager_GoNextPage(&screen_obj);
      else
        screen_obj.coming_call_page.is_accept_call = !screen_obj.coming_call_page.is_accept_call;
    }
#ifdef JR_DEBUG
    ++i;
#endif
  }
  ScreenManager_Dtor(&screen_obj);
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
  for(;;)
  {
    // Count steps
#ifndef JR_DEBUG
    ADXL345_DoStepCnt(&adxl345_obj);
#endif
  }
  /* USER CODE END BackendRun */
}

/* Private application code --------------------------------------------------*/
/* USER CODE BEGIN Application */
void HardWare_Init()
{
  // Init Screen
  ScreenManager_Ctor(&screen_obj);
  // Init MAX30102
  MAX30102_Init(&max30102_obj);
  // Init ADXL345
#ifndef JR_DEBUG
  ADXL345_Init(&adxl345_obj);
#endif
  // Init HC-06
  HC06_Init(&hc06_obj);
  // Init Coming call handler
  ComingCallHandler_Init(&coming_call_handler_obj, &hc06_obj);
}

void HAL_UART_RxCpltCallback(UART_HandleTypeDef *huart)
{
  uint8_t msg_len;
  char msg[MSG_SIZE];
  switch (HC06_HandleMsg(&hc06_obj, msg, &msg_len))
  {
  case MSG_TYPE_COMING_CALL_NOTIFY:

    ComingCallHandler_NewCallNotify(&coming_call_handler_obj, msg, msg_len);
    break;

  case MSG_TYPE_CALL_HANGUP_NOTIFY:
    ComingCallHandler_PeerHangupNotify(&coming_call_handler_obj);
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
    HC06_SendString(&hc06_obj, (char*)&ch, 1);
    return ch;
}
/* USER CODE END Application */

