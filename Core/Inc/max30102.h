#ifndef __MAX30102_H__
#define __MAX30102_H__

#include <stdint.h>

#define CACHE_NUMS 150//缓存数
#define PPG_DATA_THRESHOLD 100 	//检测阈值

typedef struct
{
  int32_t n_sp02; //SPO2 value 
  int32_t n_heart_rate;   //heart rate value 
  uint8_t max30102_int_flag;  		//中断标志
  float ppg_data_cache_RED[CACHE_NUMS];  //缓存区
  float ppg_data_cache_IR[CACHE_NUMS];  //缓存区
  uint16_t cache_counter;  //缓存计数器
	float max30102_data[2], fir_output[2];
} max30102_t;

void MAX30102_Init(max30102_t* obj);
void MAX30102_DoSample(max30102_t* obj);
void MAX30102_GetData(max30102_t* obj, int32_t* hr, int32_t* spo2);

#endif
