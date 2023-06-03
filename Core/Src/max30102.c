#include "max30102.h"
#include "algorithm.h"
#include "i2c.h"
#include "main.h"
#include "max30102_for_stm32_hal.h"

#define MAX_BRIGHTNESS 255
#define HEART_RATE_POINT_SIZE 128

void MAX30102_Init(max30102_t* obj)
{
  maxim_max30102_reset(); //resets the MAX30102
	//read and clear status register
	maxim_max30102_read_reg(0, &obj->uch_dummy);
	maxim_max30102_init();  //initializes the MAX30102

	obj->n_brightness = 0;
	obj->un_min = 0x3FFFF;
	obj->un_max = 0;
	
	//read the first 500 samples, and determine the signal range
  int i;
	for (i = 0; i < IR_RED_BUFFER_LENGTH; ++i)
	{
		while (1 == HAL_GPIO_ReadPin(HE_INT_GPIO_Port, HE_INT_Pin));   //wait until the interrupt pin asserts
		maxim_max30102_read_fifo((obj->aun_red_buffer + i), (obj->aun_ir_buffer + i));  //read from MAX30102 FIFO
		if (obj->un_min > obj->aun_red_buffer[i])
			obj->un_min = obj->aun_red_buffer[i];    //update signal min
		if (obj->un_max < obj->aun_red_buffer[i])
			obj->un_max = obj->aun_red_buffer[i];    //update signal max
	}
	obj->un_prev_data = obj->aun_red_buffer[i];
	//calculate heart rate and SpO2 after first 500 samples (first 5 seconds of samples)
	maxim_heart_rate_and_oxygen_saturation(obj->aun_ir_buffer, IR_RED_BUFFER_LENGTH, obj->aun_red_buffer, 
  &obj->n_sp02, &obj->ch_spo2_valid, &obj->n_heart_rate, &obj->ch_hr_valid); 
}

void MAX30102_DoSample(max30102_t* obj)
{
	int i = 0;
	obj->un_min = 0x3FFFF;
  obj->un_max = 0;
  //dumping the first 100 sets of samples in the memory and shift the last 400 sets of samples to the top
  for (i = 100; i < IR_RED_BUFFER_LENGTH; ++i)
  {
    obj->aun_red_buffer[i - 100] = obj->aun_red_buffer[i];
    obj->aun_ir_buffer[i - 100] = obj->aun_ir_buffer[i];
    //update the signal min and max
    if (obj->un_min > obj->aun_red_buffer[i])
      obj->un_min = obj->aun_red_buffer[i];
    if (obj->un_max < obj->aun_red_buffer[i])
      obj->un_max = obj->aun_red_buffer[i];
  }
  //take 100 sets of samples before calculating the heart rate.
  for (i = 400; i < IR_RED_BUFFER_LENGTH; ++i)
  {
    obj->un_prev_data = obj->aun_red_buffer[i - 1];
    while (1 == HAL_GPIO_ReadPin(HE_INT_GPIO_Port, HE_INT_Pin));   //wait until the interrupt pin asserts
    maxim_max30102_read_fifo((obj->aun_red_buffer + i), (obj->aun_ir_buffer + i));
    if (obj->aun_red_buffer[i] > obj->un_prev_data)  //just to determine the brightness of LED according to the deviation of adjacent two AD data
    {
      obj->f_temp = obj->aun_red_buffer[i] - obj->un_prev_data;
      obj->f_temp /= (obj->un_max - obj->un_min);
      obj->f_temp *= MAX_BRIGHTNESS;
      obj->n_brightness -= (int)obj->f_temp;
      if (obj->n_brightness < 0)
        obj->n_brightness = 0;
    }
    else
    {
      obj->f_temp = obj->un_prev_data - obj->aun_red_buffer[i];
      obj->f_temp /= (obj->un_max - obj->un_min);
      obj->f_temp *= MAX_BRIGHTNESS;
      obj->n_brightness += (int)obj->f_temp;
      if (obj->n_brightness > MAX_BRIGHTNESS)
        obj->n_brightness = MAX_BRIGHTNESS;
    }
  }
  maxim_heart_rate_and_oxygen_saturation(obj->aun_ir_buffer, IR_RED_BUFFER_LENGTH, obj->aun_red_buffer, 
  &obj->n_sp02, &obj->ch_spo2_valid, &obj->n_heart_rate, &obj->ch_hr_valid); 
}

void MAX30102_GetData(max30102_t* obj, int32_t* hr, int32_t* spo2)
{
  *hr = obj->ch_hr_valid ? obj->n_heart_rate : 0;
  *spo2 = obj->ch_spo2_valid ? obj->n_sp02 : 0;
}
