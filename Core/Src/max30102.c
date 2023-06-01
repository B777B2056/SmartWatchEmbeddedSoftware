#include "max30102.h"
#include "algorithm.h"
#include "i2c.h"

#define CALCULATE_SET_N 25

typedef struct
{
  max30102_t _max30102;
  int8_t init_buf_idx;
  uint32_t ir_buffer[SAMPLE_N];
  uint32_t red_buffer[SAMPLE_N];

  uint8_t shift_flag;
  int8_t buf_tail_write_idx;

  int32_t _spo2; int8_t _spo2_valid;
  int32_t _heart_rate; int8_t  _hr_valid;
} MAX30102SensorPrivateMember;

#define PRIVATE_MEMBER(obj) ((MAX30102SensorPrivateMember*)((obj)->_private_member))

static void MAX30102SensorInit(struct MAX30102_SENSOR* obj, MAX30102SensorPrivateMember* member)
{
  if (!obj || !member) return;
  
  member->_max30102._sensor = obj;
  member->init_buf_idx = 0;

  member->shift_flag = 1;
  member->buf_tail_write_idx = SAMPLE_N - CALCULATE_SET_N;

  member->_spo2 = -1;
  member->_spo2_valid = 0;
  member->_heart_rate = -1;
  member->_hr_valid = 0;

  obj->_private_member = member;
  obj->HasInterrupt = MAX30102SensorHasInterrupt;
  obj->HandleInterrupt = MAX30102SensorHandleInterrupt;
  obj->OnInterrupt = MAX30102SensorOnInterrupt;
  obj->DoCalculate = MAX30102SensorDoCalculate;

  max30102_init(&member->_max30102, &hi2c2);
  // 重置MAX30102传感器
  max30102_reset(&member->_max30102);
  max30102_clear_fifo(&member->_max30102);
  // 配置FIFO
  max30102_set_fifo_config(&member->_max30102, max30102_smp_ave_8, 1, 7);
  // 配置LED
  max30102_set_led_pulse_width(&member->_max30102, max30102_pw_16_bit);
  max30102_set_adc_resolution(&member->_max30102, max30102_adc_2048);
  max30102_set_sampling_rate(&member->_max30102, max30102_sr_800);
  max30102_set_led_current_1(&member->_max30102, 6.2);
  max30102_set_led_current_2(&member->_max30102, 6.2);
  // 初始化SpO2模式（）
  max30102_set_mode(&member->_max30102, max30102_spo2);
  // 使能FIFO_A_FULL中断
  max30102_set_a_full(&member->_max30102, 1);
  // 不使能温度测量
  max30102_set_die_temp_en(&member->_max30102, 0);
  // 不使能DIE_TEMP_RDY中断
  max30102_set_die_temp_rdy(&member->_max30102, 0);
}

MAX30102Sensor* MAX30102SensorInstance()
{
  static uint8_t flag = 0;
  static MAX30102Sensor sensor;
  static MAX30102SensorPrivateMember member;
  if (!flag)
  {
    MAX30102SensorInit(&sensor, &member);
    flag = 1;
  }
  return &sensor;
}

uint8_t MAX30102SensorHasInterrupt(struct MAX30102_SENSOR* obj)
{
  return max30102_has_interrupt(&PRIVATE_MEMBER(obj)->_max30102);
}

void MAX30102SensorHandleInterrupt(struct MAX30102_SENSOR* obj, int32_t* heart_rate, int32_t* spo2)
{
  max30102_interrupt_handler(&PRIVATE_MEMBER(obj)->_max30102);

  *heart_rate = (PRIVATE_MEMBER(obj)->_hr_valid ? PRIVATE_MEMBER(obj)->_heart_rate : -1);
  *spo2 = (PRIVATE_MEMBER(obj)->_spo2_valid ? PRIVATE_MEMBER(obj)->_spo2 : -1);
}

void MAX30102SensorOnInterrupt(struct MAX30102_SENSOR* obj)
{
  max30102_on_interrupt(&PRIVATE_MEMBER(obj)->_max30102);
}

void MAX30102SensorDoCalculate(struct MAX30102_SENSOR* obj)
{
  maxim_heart_rate_and_oxygen_saturation(PRIVATE_MEMBER(obj)->ir_buffer, SAMPLE_N, PRIVATE_MEMBER(obj)->red_buffer, &PRIVATE_MEMBER(obj)->_spo2, &PRIVATE_MEMBER(obj)->_spo2_valid, &PRIVATE_MEMBER(obj)->_heart_rate, &PRIVATE_MEMBER(obj)->_hr_valid);
}

// 覆写获取ir和red值的函数
void max30102_plot(max30102_t* obj, uint32_t ir_sample, uint32_t red_sample)
{
  struct MAX30102_SENSOR* sensor = obj->_sensor;
  if (PRIVATE_MEMBER(sensor)->init_buf_idx < SAMPLE_N)
  {
    // 初始运行时先填满ir和red缓冲区
    PRIVATE_MEMBER(sensor)->ir_buffer[PRIVATE_MEMBER(sensor)->init_buf_idx] = ir_sample;
    PRIVATE_MEMBER(sensor)->red_buffer[PRIVATE_MEMBER(sensor)->init_buf_idx] = red_sample;
    ++PRIVATE_MEMBER(sensor)->init_buf_idx;
    if (SAMPLE_N == PRIVATE_MEMBER(sensor)->init_buf_idx)
      sensor->DoCalculate(sensor); // 计算心率和血氧（第一次计算）
  }
  else
  {
    if (PRIVATE_MEMBER(sensor)->shift_flag)
    {
      // 左移（以CALCULATE_SET_N为单位进行缓冲区出队）
      int8_t i = CALCULATE_SET_N;
      for (; i < SAMPLE_N; ++i)
      {
        PRIVATE_MEMBER(sensor)->ir_buffer[i - CALCULATE_SET_N] = PRIVATE_MEMBER(sensor)->ir_buffer[i];
        PRIVATE_MEMBER(sensor)->red_buffer[i - CALCULATE_SET_N] = PRIVATE_MEMBER(sensor)->red_buffer[i];
      }
      // 置标志位
      PRIVATE_MEMBER(sensor)->shift_flag = 0;
    }
    else
    {
      // 从buf_tail_write_idx位置开始填充缓冲区右侧（以CALCULATE_SET_N为单位进行缓冲区入队）
      PRIVATE_MEMBER(sensor)->ir_buffer[PRIVATE_MEMBER(sensor)->buf_tail_write_idx] = ir_sample;
      PRIVATE_MEMBER(sensor)->red_buffer[PRIVATE_MEMBER(sensor)->buf_tail_write_idx] = red_sample;
      ++PRIVATE_MEMBER(sensor)->buf_tail_write_idx;
      // 填充已满，重新执行缓冲区左移
      if (SAMPLE_N == PRIVATE_MEMBER(sensor)->buf_tail_write_idx)
      {
        PRIVATE_MEMBER(sensor)->buf_tail_write_idx = SAMPLE_N - CALCULATE_SET_N;
        PRIVATE_MEMBER(sensor)->shift_flag = 1;
        // 计算心率和血氧（第一次计算）
        sensor->DoCalculate(sensor);
      }
    }
  }
}
