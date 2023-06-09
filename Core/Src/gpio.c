/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file    gpio.c
  * @brief   This file provides code for the configuration
  *          of all used GPIO pins.
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
#include "gpio.h"

/* USER CODE BEGIN 0 */
#include "cmsis_os.h"
/* USER CODE END 0 */

/*----------------------------------------------------------------------------*/
/* Configure GPIO                                                             */
/*----------------------------------------------------------------------------*/
/* USER CODE BEGIN 1 */

/* USER CODE END 1 */

/** Configure pins as
        * Analog
        * Input
        * Output
        * EVENT_OUT
        * EXTI
*/
void MX_GPIO_Init(void)
{

  GPIO_InitTypeDef GPIO_InitStruct = {0};

  /* GPIO Ports Clock Enable */
  __HAL_RCC_GPIOC_CLK_ENABLE();
  __HAL_RCC_GPIOD_CLK_ENABLE();
  __HAL_RCC_GPIOA_CLK_ENABLE();
  __HAL_RCC_GPIOB_CLK_ENABLE();

  /*Configure GPIO pin Output Level */
  HAL_GPIO_WritePin(BEEP_GPIO_Port, BEEP_Pin, GPIO_PIN_SET);

  /*Configure GPIO pin : PtPin */
  GPIO_InitStruct.Pin = HE_INT_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_IT_FALLING;
  GPIO_InitStruct.Pull = GPIO_PULLUP;
  HAL_GPIO_Init(HE_INT_GPIO_Port, &GPIO_InitStruct);

  /*Configure GPIO pin : PtPin */
  GPIO_InitStruct.Pin = BEEP_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  HAL_GPIO_Init(BEEP_GPIO_Port, &GPIO_InitStruct);

  /*Configure GPIO pins : PAPin PAPin */
  GPIO_InitStruct.Pin = KEY_CONFIRM_Pin|KEY_PAGE_CHOOSE_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_INPUT;
  GPIO_InitStruct.Pull = GPIO_PULLUP;
  HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);

  /* EXTI interrupt init*/
  HAL_NVIC_SetPriority(EXTI0_IRQn, 5, 0);
  HAL_NVIC_EnableIRQ(EXTI0_IRQn);

}

/* USER CODE BEGIN 2 */
void KeyInit(user_key_t* this, KeyType t)
{
  this->type = t;
  this->count = 0;
  this->cur_status = KS_RELEASE;
  this->is_short_pressed = this->is_long_pressed = false;
}

static GPIO_PinState CheckGPIO_PinState(user_key_t* this)
{
  switch (this->type)
  {
  case PAGE_CHOOSE_KEY:
    return HAL_GPIO_ReadPin(KEY_PAGE_CHOOSE_GPIO_Port, KEY_PAGE_CHOOSE_Pin);

  case CONFIRM_KEY:
    return HAL_GPIO_ReadPin(KEY_CONFIRM_GPIO_Port, KEY_CONFIRM_Pin);
  
  default:
    return GPIO_PIN_SET;
  }
}

/* Detect key is or is not pressed */
uint8_t KeyScan(user_key_t* this)
{
  uint8_t ret = KEY_OFF;
  switch (this->cur_status)
  {
  case KS_RELEASE:
    if (this->is_short_pressed)
    {
      this->is_short_pressed = false;
      ret = KEY_ON;
    }
    if (this->is_long_pressed)
    {
      this->is_long_pressed = false;
      ret = KEY_LONG_ON;
    }
    if (GPIO_PIN_RESET == CheckGPIO_PinState(this))
    {
      this->cur_status = KS_SHAKE;
    }
    break;

  case KS_SHAKE:
    if (GPIO_PIN_RESET == CheckGPIO_PinState(this))
    {
      this->cur_status = KS_SHORT_PRESS;
    }
    else
    {
      this->cur_status = KS_RELEASE;
    }
    break;

  case KS_SHORT_PRESS:
    if (GPIO_PIN_SET == CheckGPIO_PinState(this))
    {
      this->count = 0;
      this->is_short_pressed = true;
      this->cur_status = KS_SHAKE;
    }
    else
    {
      ++this->count;
      if (this->count < KEY_LONG_PRESS_THRESHOLD_MS/KEY_TIMER_PERIOD_MS)
        this->cur_status = KS_SHORT_PRESS;
      else
        this->cur_status = KS_LONG_PRESS;
    }
    break;

  case KS_LONG_PRESS:
    this->count = 0;
    if (GPIO_PIN_SET == CheckGPIO_PinState(this))
    {
      this->is_long_pressed = true;
      this->cur_status = KS_SHAKE;
    }
    else
      this->cur_status = KS_LONG_PRESS;
    break;
  
  default:
    break;
  }
  return ret;
}
/* USER CODE END 2 */
