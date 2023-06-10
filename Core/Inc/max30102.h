#ifndef __MAX30102_H__
#define __MAX30102_H__

#include <stdint.h>

#define MAX_BRIGHTNESS 255
#define START 20
#define DATA_LENGTH 100

typedef enum
{
  MA_MOVE_PRE = 0,
  MA_WAIT_INT,
  MA_FILL
} MAX30102_STATUS;

typedef struct
{
  uint32_t aun_ir_buffer[DATA_LENGTH]; //IR LED sensor data
  int32_t n_ir_buffer_length;    //data length
  uint32_t aun_red_buffer[DATA_LENGTH];    //Red LED sensor data
  int32_t n_spo2; //SPO2 value
  int8_t ch_spo2_valid;   //indicator to show if the SP02 calculation is valid
  int32_t n_heart_rate;   //heart rate value
  int8_t  ch_hr_valid;    //indicator to show if the heart rate calculation is valid
  uint8_t uch_dummy;
  uint32_t un_min, un_max, un_prev_data;  //variables to calculate the on-board LED brightness that reflects the heartbeats
	int32_t n_brightness;
	float f_temp;
  uint16_t write_size, write_idx;
  MAX30102_STATUS cur_status;
} max30102_t;

void MAX30102_Init(max30102_t* obj);
void MAX30102_DoSample(max30102_t* obj);
void MAX30102_GetData(max30102_t* obj, int32_t* hr, int32_t* spo2);

#endif
