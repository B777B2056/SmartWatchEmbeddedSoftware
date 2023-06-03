#ifndef __ADXL345_H__
#define __ADXL345_H__
#include <stdint.h>

#define ACC_BUFFER_LEN  8

typedef struct
{
  uint32_t step_count;
  uint8_t acc_buf_cur_idx;
  float acc_buffer[ACC_BUFFER_LEN];
} adxl345_t;


void ADXL345_Init(adxl345_t* obj);
void ADXL345_DoStepCnt(adxl345_t* obj);
uint32_t ADXL345_GetSteps(adxl345_t* obj);

#endif
