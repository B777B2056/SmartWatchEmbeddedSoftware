#include "max30102.h"
#include "algorithm.h"
#include "i2c.h"
#include "main.h"
#include "max30102_for_stm32_hal.h"

#define MAX_BRIGHTNESS 255
#define IR_RED_BUFFER_LENGTH 500
#define HEART_RATE_POINT_SIZE 128

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

void MAX30102_Init()
{
  maxim_max30102_reset(); //resets the MAX30102
	//read and clear status register
	maxim_max30102_read_reg(0, &uch_dummy);
	maxim_max30102_init();  //initializes the MAX30102

	n_brightness = 0;
	un_min = 0x3FFFF;
	un_max = 0;
	
	//read the first 500 samples, and determine the signal range
  int i;
	for(i = 0;i < IR_RED_BUFFER_LENGTH; ++i)
	{
		while (1 == HAL_GPIO_ReadPin(HE_INT_GPIO_Port, HE_INT_Pin));   //wait until the interrupt pin asserts
		maxim_max30102_read_fifo((aun_red_buffer + i), (aun_ir_buffer + i));  //read from MAX30102 FIFO
		if (un_min > aun_red_buffer[i])
			un_min = aun_red_buffer[i];    //update signal min
		if (un_max < aun_red_buffer[i])
			un_max = aun_red_buffer[i];    //update signal max
	}
	un_prev_data = aun_red_buffer[i];
	//calculate heart rate and SpO2 after first 500 samples (first 5 seconds of samples)
	maxim_heart_rate_and_oxygen_saturation(aun_ir_buffer, IR_RED_BUFFER_LENGTH, aun_red_buffer, &n_sp02, &ch_spo2_valid, &n_heart_rate, &ch_hr_valid); 
}

void MAX30102_DoSample()
{
	int i = 0;
	un_min = 0x3FFFF;
  un_max = 0;
  //dumping the first 100 sets of samples in the memory and shift the last 400 sets of samples to the top
  for (i = 100; i < IR_RED_BUFFER_LENGTH; ++i)
  {
    aun_red_buffer[i - 100] = aun_red_buffer[i];
    aun_ir_buffer[i - 100] = aun_ir_buffer[i];
    //update the signal min and max
    if (un_min > aun_red_buffer[i])
      un_min = aun_red_buffer[i];
    if (un_max < aun_red_buffer[i])
      un_max = aun_red_buffer[i];
  }
  //take 100 sets of samples before calculating the heart rate.
  for(i = 400; i < IR_RED_BUFFER_LENGTH; ++i)
  {
    un_prev_data = aun_red_buffer[i - 1];
    while (1 == HAL_GPIO_ReadPin(HE_INT_GPIO_Port, HE_INT_Pin));   //wait until the interrupt pin asserts
    maxim_max30102_read_fifo((aun_red_buffer + i), (aun_ir_buffer + i));
    if(aun_red_buffer[i] > un_prev_data)  //just to determine the brightness of LED according to the deviation of adjacent two AD data
    {
      f_temp = aun_red_buffer[i] - un_prev_data;
      f_temp /= (un_max - un_min);
      f_temp *= MAX_BRIGHTNESS;
      n_brightness -= (int)f_temp;
      if (n_brightness < 0)
        n_brightness = 0;
    }
    else
    {
      f_temp = un_prev_data - aun_red_buffer[i];
      f_temp /= (un_max - un_min);
      f_temp *= MAX_BRIGHTNESS;
      n_brightness += (int)f_temp;
      if (n_brightness > MAX_BRIGHTNESS)
        n_brightness = MAX_BRIGHTNESS;
    }
  }
  maxim_heart_rate_and_oxygen_saturation(aun_ir_buffer, IR_RED_BUFFER_LENGTH, aun_red_buffer, &n_sp02, &ch_spo2_valid, &n_heart_rate, &ch_hr_valid); 
}

void MAX30102_GetData(int32_t* hr, int32_t* spo2)
{
  *hr = ch_hr_valid ? n_heart_rate : 0;
  *spo2 = ch_spo2_valid ? n_sp02 : 0;
}
