/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file    gpio.h
  * @brief   This file contains all the function prototypes for
  *          the gpio.c file
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
/* Define to prevent recursive inclusion -------------------------------------*/
#ifndef __GPIO_H__
#define __GPIO_H__

#ifdef __cplusplus
extern "C" {
#endif

/* Includes ------------------------------------------------------------------*/
#include "main.h"

/* USER CODE BEGIN Includes */
#include <stdint.h>
#include <stdbool.h>
/* USER CODE END Includes */

/* USER CODE BEGIN Private defines */
#define KEY_OFF     0
#define KEY_ON      1
#define KEY_LONG_ON 2

typedef enum
{
  PAGE_CHOOSE_KEY = 0x00,
  CONFIRM_KEY
} KeyType;

#define ENUM_ITEM(ITEM) ITEM,

#define KEY_STATUS_ENUM(STATUS) \
    STATUS(KS_RELEASE)  \
    STATUS(KS_SHAKE)  \
    STATUS(KS_SHORT_PRESS)  \
    STATUS(KS_LONG_PRESS)  \
    STATUS(KS_NUM)  \
    
typedef enum
{
  KEY_STATUS_ENUM(ENUM_ITEM)
} KEY_STATUS;

typedef struct
{
  KeyType type;
  uint8_t count;
  bool is_short_pressed, is_long_pressed;
  KEY_STATUS cur_status;
} user_key_t;

/* USER CODE END Private defines */

void MX_GPIO_Init(void);

/* USER CODE BEGIN Prototypes */
void KeyInit(user_key_t* this, KeyType t);
uint8_t KeyScan(user_key_t* this);
/* USER CODE END Prototypes */

#ifdef __cplusplus
}
#endif
#endif /*__ GPIO_H__ */

