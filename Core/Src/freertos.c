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
  // Init OLED
  OLED_Init();
  // Init MAX30102
  MAX30102Sensor* healthy_sensor = MAX30102SensorInstance();
  // Wait bluetooth init
  osSemaphoreWait(taskInitCountingSemHandle, osWaitForever);
  /* Infinite loop */
  for(;;)
  {
    switch (OLED_CurrentPage())
    {
    case DATE_PAGE:
      hasComingCall = false;
      OLED_ShowDate();
      break;

    case HEALTHY_PAGE:
      hasComingCall = false;
      // Get MAX30102's data
      int32_t heart_rate = -1;
      int32_t spo2 = -1;
      if (healthy_sensor->HasInterrupt(healthy_sensor))
      {
        healthy_sensor->HandleInterrupt(healthy_sensor, &heart_rate, &spo2);
      }
      OLED_ShowHealthy(heart_rate, spo2);
      break;

    case STEP_CNT_PAGE:
      hasComingCall = false;
      OLED_ShowStepCnt(ADXL345_GetSteps());
      break;

    case COMING_CALL_PAGE:
      hasComingCall = true;
      OLED_ShowComingCall(HC06_GetRecvedMsg(), isAcceptComingCall);
      if (KEY_ON == KeyScan(CONFIRM_KEY))
      {
        // Notify bluetooth to do work
        HC06_ComingCallOption(isAcceptComingCall);
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
        OLED_GoNextPage();
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
  // Init ADXL345
  ADXL345_Init();
  // Init HC-06
  HC06_WaitMsg();
  osSemaphoreRelease(taskInitCountingSemHandle);
  /* Infinite loop */
  for(;;)
  {
    // Call coming
    if (HC06_IsNewMsgRecved())
      OLED_SetCurrentPage(COMING_CALL_PAGE);
    // Call cancel
    if (HC06_ComingCallIsHangupByPeer())
      OLED_SetCurrentPage(DATE_PAGE);
    // Count steps
    ADXL345_DoStepCnt();
    osDelay(50);
  }
  /* USER CODE END BackendRun */
}

/* Private application code --------------------------------------------------*/
/* USER CODE BEGIN Application */

/* USER CODE END Application */

