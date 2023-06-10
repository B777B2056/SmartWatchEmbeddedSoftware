#include "max30102.h"
#include "i2c.h"
#include "gpio.h"
#include "main.h"
#include "cmsis_os.h"
#include "max30102_hal.h"
#include "max30102_algorithm.h"

extern osMutexId healthyGetterMutexHandle;

void MAX30102_Init(max30102_t* obj)
{
  maxim_max30102_reset(); //resets the MAX30102
	maxim_max30102_read_reg(0, &obj->uch_dummy);  //read and clear status register
	maxim_max30102_init(&hi2c2);  //initializes the MAX30102

  obj->n_brightness = 0;
	obj->un_min = 0x3FFFF;
	obj->un_max = 0;
	obj->n_ir_buffer_length = DATA_LENGTH; //buffer length of 100 stores 5 seconds of samples running at 100sps
  obj->write_size = obj->write_idx = 0;
  obj->cur_status = MA_WAIT_INT;
}

void MAX30102_DoSample(max30102_t* obj)
{
  int32_t n_spo2;
  int8_t ch_spo2_valid;
  int32_t n_heart_rate;
  int8_t  ch_hr_valid;
  if (obj->write_size < DATA_LENGTH)
  {
    switch (obj->cur_status)
    {
    case MA_WAIT_INT:
      if (GPIO_PIN_RESET == HAL_GPIO_ReadPin(HE_INT_GPIO_Port, HE_INT_Pin))
      {
        obj->cur_status = MA_FILL;
      }
      break;

    case MA_FILL:
      {
        maxim_max30102_read_fifo((obj->aun_red_buffer+obj->write_idx), (obj->aun_ir_buffer+obj->write_idx));  //read from MAX30102 FIFO	
        if (obj->un_min > obj->aun_red_buffer[obj->write_idx])
          obj->un_min = obj->aun_red_buffer[obj->write_idx];    //update signal min
        if (obj->un_max < obj->aun_red_buffer[obj->write_idx])
          obj->un_max = obj->aun_red_buffer[obj->write_idx];    //update signal max
        ++obj->write_idx;
        ++obj->write_size;
        if (DATA_LENGTH == obj->write_idx)
        {
          obj->un_prev_data = obj->aun_red_buffer[obj->write_idx];
          //calculate heart rate and SpO2 after first 500 samples (first 5 seconds of samples)
          maxim_heart_rate_and_oxygen_saturation(obj->aun_ir_buffer, obj->n_ir_buffer_length, obj->aun_red_buffer, &n_spo2, &ch_spo2_valid, &n_heart_rate, &ch_hr_valid); 
          obj->cur_status = MA_MOVE_PRE;
        }
        else
          obj->cur_status = MA_WAIT_INT;
      }
      break;
    
    default:
      break;
    }
  }
  else
  {
    switch (obj->cur_status)
    {
    case MA_MOVE_PRE:
      {
        obj->un_min = 0x3FFFF;
        obj->un_max = 0;
        //dumping the first 100 sets of samples in the memory and shift the last 400 sets of samples to the top
        for (obj->write_idx = START; obj->write_idx < DATA_LENGTH; ++obj->write_idx)
        {
          obj->aun_red_buffer[obj->write_idx-START] = obj->aun_red_buffer[obj->write_idx];
          obj->aun_ir_buffer[obj->write_idx-START] = obj->aun_ir_buffer[obj->write_idx];
          
          //update the signal min and max
          if (obj->un_min > obj->aun_red_buffer[obj->write_idx])
            obj->un_min = obj->aun_red_buffer[obj->write_idx];
          if (obj->un_max < obj->aun_red_buffer[obj->write_idx])
            obj->un_max = obj->aun_red_buffer[obj->write_idx];
        }
        obj->write_idx = DATA_LENGTH - START;
        obj->cur_status = MA_WAIT_INT;
      }
      break;

    case MA_WAIT_INT:
      if (GPIO_PIN_RESET == HAL_GPIO_ReadPin(HE_INT_GPIO_Port, HE_INT_Pin))
      {
        obj->cur_status = MA_FILL;
      }
      break;

    case MA_FILL:
      {
        obj->un_prev_data = obj->aun_red_buffer[obj->write_idx-1];
        maxim_max30102_read_fifo((obj->aun_red_buffer+obj->write_idx), (obj->aun_ir_buffer+obj->write_idx));
        if (obj->aun_red_buffer[obj->write_idx] > obj->un_prev_data)//just to determine the brightness of LED according to the deviation of adjacent two AD data
        {
          obj->f_temp = obj->aun_red_buffer[obj->write_idx] - obj->un_prev_data;
          obj->f_temp /= (obj->un_max - obj->un_min);
          obj->f_temp *= MAX_BRIGHTNESS;
          obj->n_brightness -= (int)obj->f_temp;
          if(obj->n_brightness < 0)
            obj->n_brightness = 0;
        }
        else
        {
          obj->f_temp = obj->un_prev_data - obj->aun_red_buffer[obj->write_idx];
          obj->f_temp /= (obj->un_max - obj->un_min);
          obj->f_temp *= MAX_BRIGHTNESS;
          obj->n_brightness += (int)obj->f_temp;
          if(obj->n_brightness > MAX_BRIGHTNESS)
            obj->n_brightness = MAX_BRIGHTNESS;
        }
        ++obj->write_idx;
        if (DATA_LENGTH == obj->write_idx)
        {
          maxim_heart_rate_and_oxygen_saturation(obj->aun_ir_buffer, obj->n_ir_buffer_length, obj->aun_red_buffer, &n_spo2, &ch_spo2_valid, &n_heart_rate, &ch_hr_valid); 
          obj->cur_status = MA_MOVE_PRE;
        }
        else
          obj->cur_status = MA_WAIT_INT;
      }
      break;
    
    default:
      break;
    }
  }
  osMutexWait(healthyGetterMutexHandle, osWaitForever);
  obj->n_spo2 = n_spo2; obj->ch_spo2_valid = ch_spo2_valid;
  obj->n_heart_rate = n_heart_rate; obj->ch_hr_valid = ch_hr_valid;
  osMutexRelease(healthyGetterMutexHandle);
}

void MAX30102_GetData(max30102_t* obj, int32_t* hr, int32_t* spo2)
{
  osMutexWait(healthyGetterMutexHandle, osWaitForever);
  if (obj->ch_hr_valid && (obj->n_heart_rate >= 0) && (obj->n_heart_rate < 150))
    *hr = obj->n_heart_rate;
  if (obj->ch_spo2_valid && (obj->n_spo2 >= 0) && (obj->n_spo2 <= 100))
    *spo2 = obj->n_spo2;
  osMutexRelease(healthyGetterMutexHandle);
}
