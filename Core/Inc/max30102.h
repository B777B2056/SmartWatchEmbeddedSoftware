#ifndef __MAX30102_H__
#define __MAX30102_H__

#include "max30102_for_stm32_hal.h"

#define SAMPLE_N 100

typedef struct MAX30102_SENSOR
{
  void* _private_member;

  uint8_t (*HasInterrupt)(struct MAX30102_SENSOR* obj);
  void (*HandleInterrupt)(struct MAX30102_SENSOR* obj, int32_t* heart_rate, int32_t* spo2);
  void (*OnInterrupt)(struct MAX30102_SENSOR* obj);
  void (*DoCalculate)(struct MAX30102_SENSOR* obj);
} MAX30102Sensor;

MAX30102Sensor* MAX30102SensorInstance();
uint8_t MAX30102SensorHasInterrupt(struct MAX30102_SENSOR* obj);
void MAX30102SensorHandleInterrupt(struct MAX30102_SENSOR* obj, int32_t* heart_rate, int32_t* spo2);
void MAX30102SensorOnInterrupt(struct MAX30102_SENSOR* obj);
void MAX30102SensorDoCalculate(struct MAX30102_SENSOR* obj);

#endif
