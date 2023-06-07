#include "max30102.h"
#include "i2c.h"
#include "main.h"
#include "max30102_fir.h"
#include "max30102_g.h"
#include "gpio.h"

void MAX30102_Init(max30102_t* obj)
{
  obj->max30102_int_flag = 0;
  obj->cache_counter = 0;
  max30102_init();
	max30102_fir_init();
}

void MAX30102_DoSample(max30102_t* obj)
{
  if(GPIO_PIN_RESET == HAL_GPIO_ReadPin(HE_INT_GPIO_Port, HE_INT_Pin))			//中断信号产生
  {
      // obj->max30102_int_flag = 0;
      max30102_fifo_read(obj->max30102_data);		//读取数据
    
      ir_max30102_fir(&obj->max30102_data[0], &obj->fir_output[0]);
      red_max30102_fir(&obj->max30102_data[1], &obj->fir_output[1]);  //滤波
      if((obj->max30102_data[0] > PPG_DATA_THRESHOLD) && (obj->max30102_data[1] > PPG_DATA_THRESHOLD))  //大于阈值，说明传感器有接触
      {		
          obj->ppg_data_cache_IR[obj->cache_counter] = obj->fir_output[0];
          obj->ppg_data_cache_RED[obj->cache_counter] = obj->fir_output[1];
          obj->cache_counter++;
      }
      else				//小于阈值
      {
          obj->cache_counter = 0;
      }

      if(obj->cache_counter >= CACHE_NUMS)  //收集满了数据
      {
        obj->n_heart_rate = max30102_getHeartRate(obj->ppg_data_cache_IR,CACHE_NUMS);
        obj->n_sp02 = max30102_getSpO2(obj->ppg_data_cache_IR,obj->ppg_data_cache_RED,CACHE_NUMS);
        obj->cache_counter=0;
      }
  }
}

void MAX30102_GetData(max30102_t* obj, int32_t* hr, int32_t* spo2)
{
  if ((obj->n_heart_rate >= 0) && (obj->n_heart_rate <= 200))
    *hr = obj->n_heart_rate;
  if ((obj->n_sp02 >= 0) && (obj->n_sp02 <= 100))
    *spo2 = obj->n_sp02;
}
