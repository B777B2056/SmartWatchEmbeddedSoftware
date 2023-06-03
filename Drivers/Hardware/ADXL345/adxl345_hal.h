#ifndef __ADXL345_HAL_H__
#define __ADXL345_HAL_H__

#include "stm32f1xx_hal.h"
#include <stdint.h>

void ADXL345_Driver_Init(I2C_HandleTypeDef* hi2c);
void ADXL345_Driver_Write(uint8_t addr, uint8_t value);
void ADXL345_Driver_Read(uint8_t addr, uint8_t* vvalue);
void ADXL345_Driver_Axis_Data(float* acc_x, float* acc_y, float* acc_z);

#endif
