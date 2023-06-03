#ifndef __MAX30102_H__
#define __MAX30102_H__

#include <stdint.h>

#define IR_RED_BUFFER_LENGTH 500

typedef struct
{
  uint32_t aun_ir_buffer[IR_RED_BUFFER_LENGTH]; //IR LED sensor data
  uint32_t aun_red_buffer[IR_RED_BUFFER_LENGTH];    //Red LED sensor data
  int32_t n_sp02; //SPO2 value 
  int8_t ch_spo2_valid;   //indicator to show if the SP02 calculation is valid 
  int32_t n_heart_rate;   //heart rate value 
  int8_t  ch_hr_valid;    //indicator to show if the heart rate calculation is valid
  uint8_t uch_dummy;

  uint32_t un_min, un_max, un_prev_data;  //variables to calculate the on-board LED brightness that reflects the heartbeats
  int32_t n_brightness;
  float f_temp;
} max30102_t;


void MAX30102_Init(max30102_t* obj);
void MAX30102_DoSample(max30102_t* obj);
void MAX30102_GetData(max30102_t* obj, int32_t* hr, int32_t* spo2);

#endif
