#ifndef __ADXL345_H__
#define __ADXL345_H__
#include <stdint.h>

void ADXL345_Init();
void ADXL345_DoStepCnt();
uint32_t ADXL345_GetSteps();

#endif
