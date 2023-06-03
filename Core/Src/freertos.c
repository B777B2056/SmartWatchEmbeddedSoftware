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
oled_t oled_obj;
max30102_t max30102_obj;
hc06_t hc06_obj;
/* USER CODE END Variables */
osThreadId frontendTaskHandle;
osThreadId backendTaskHandle;
osMutexId pageSwitchMutexHandle;
osMutexId stepCntMutexHandle;
osSemaphoreId taskInitCountingSemHandle;

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
  /* definition and creation of pageSwitchMutex */
  osMutexDef(pageSwitchMutex);
  pageSwitchMutexHandle = osMutexCreate(osMutex(pageSwitchMutex));

  /* definition and creation of stepCntMutex */
  osMutexDef(stepCntMutex);
  stepCntMutexHandle = osMutexCreate(osMutex(stepCntMutex));

  /* USER CODE BEGIN RTOS_MUTEX */
  /* add mutexes, ... */
  /* USER CODE END RTOS_MUTEX */

  /* Create the semaphores(s) */
  /* definition and creation of taskInitCountingSem */
  osSemaphoreDef(taskInitCountingSem);
  taskInitCountingSemHandle = osSemaphoreCreate(osSemaphore(taskInitCountingSem), 1);

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
  bool hasComingCall = false;
  bool isAcceptComingCall = true;
  int32_t heart_rate = 0;
  int32_t spo2 = 0;
  // Init OLED
  OLED_Init(&oled_obj);
  OLED_DisplayOn();
  OLED_ShowStartup();
  // Init MAX30102
  MAX30102_Init(&max30102_obj);
  // Init ADXL345
  ADXL345_Init(&adxl345_obj);
  // Init HC-06
  HC06_Init(&hc06_obj);
  // Wait bluetooth and ADXL345 init
  osSemaphoreRelease(taskInitCountingSemHandle);
  /* Infinite loop */
  OLED_Clear();
  for(;;)
  {
    switch (OLED_CurrentPage(&oled_obj))
    {
    case DATE_PAGE:
      hasComingCall = false;
      OLED_ShowDate();
      break;

    case HEALTHY_PAGE:
      hasComingCall = false;
      // Get MAX30102's data
      MAX30102_DoSample(&max30102_obj);
      MAX30102_GetData(&max30102_obj, &heart_rate, &spo2);
      OLED_ShowHealthy(heart_rate, spo2);
      break;

    case STEP_CNT_PAGE:
      hasComingCall = false;
      OLED_ShowStepCnt(ADXL345_GetSteps(&adxl345_obj));
      break;

    case COMING_CALL_PAGE:
      hasComingCall = true;
      OLED_ShowComingCall(HC06_GetRecvedMsg(&hc06_obj), isAcceptComingCall);
      if (KEY_ON == KeyScan(CONFIRM_KEY))
      {
        // Notify bluetooth to do work
        HC06_ComingCallOption(&hc06_obj, isAcceptComingCall);
        // Reset call flag
        hasComingCall = false;
      }
      break;
    
    default:
      hasComingCall = false;
      OLED_ShowDate();
      break;
    }
    // Detect and switch to next page
    if (KEY_ON == KeyScan(PAGE_CHOOSE_KEY))
    {
      if (!hasComingCall)
        OLED_GoNextPage(&oled_obj);
      else
        isAcceptComingCall = !isAcceptComingCall;
    }
    osDelay(50);
  }
  OLED_DisplayOff();
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
  osSemaphoreWait(taskInitCountingSemHandle, osWaitForever);
  /* Infinite loop */
  for(;;)
  {
    // Call coming
    if (HC06_IsNewMsgRecved(&hc06_obj))
      OLED_SetCurrentPage(&oled_obj, COMING_CALL_PAGE);
    // Call cancel
    if (HC06_ComingCallIsHangupByPeer(&hc06_obj))
      OLED_SetCurrentPage(&oled_obj, DATE_PAGE);
    // Count steps
    ADXL345_DoStepCnt(&adxl345_obj);
    osDelay(50);
  }
  /* USER CODE END BackendRun */
}

/* Private application code --------------------------------------------------*/
/* USER CODE BEGIN Application */
void HAL_UART_RxCpltCallback(UART_HandleTypeDef *huart)
{
  if (hc06_obj.rx_buffer_write_pos >= MSG_SIZE)  return; // 缓冲区满
    hc06_obj.has_new_msg = (MSG_END_DELIMITER == hc06_obj.ch_buffer);
  if (hc06_obj.has_new_msg)
  {
    memcpy(hc06_obj.current_msg, hc06_obj.rx_buffer, hc06_obj.rx_buffer_write_pos);
    hc06_obj.current_msg[hc06_obj.rx_buffer_write_pos] = '\0';
    hc06_obj.rx_buffer_write_pos = 0;
  }
  else
  {
    hc06_obj.rx_buffer[hc06_obj.rx_buffer_write_pos++] = hc06_obj.ch_buffer;
  }
}
/* USER CODE END Application */

