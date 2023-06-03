#ifndef __OLED_HAL_H__
#define __OLED_HAL_H__

#include "stm32f1xx_hal.h"
#include <stdint.h>

#define OLED_WIDTH 128
#define OLED_HEIGHT	8

#define CHECK_PAGE_AND_GO_NEXT(x, y)  \
    if (x > 128)  \
    { \
      x = 0;  \
      y += 2; \
    }

void OLED_Driver_Init(I2C_HandleTypeDef* hi2c);
void OLED_Driver_WriteCmd(uint8_t cmd);
void OLED_Driver_WriteChar(uint8_t data);
void OLED_Driver_SetPos(uint8_t x, uint8_t y);
void OLED_Driver_ClearTargetRow(uint8_t row);

void OLED_Driver_ShowAsciiChar(uint8_t start_x, uint8_t start_y, unsigned char ch);
void OLED_Driver_ShowAsciiString(uint8_t start_x, uint8_t start_y, const char* data);
void OLED_Driver_ShowChineseImpl(uint8_t start_x, uint8_t start_y, const uint8_t* data, uint8_t size);

#define OLED_Driver_ShowChinese(x, y, data) OLED_Driver_ShowChineseImpl(x, y, data, sizeof(data)/sizeof(data[0]))
#define OLED_Driver_ShowImage(x, y, data) OLED_Driver_ShowChinese(x, y, data)

#endif
