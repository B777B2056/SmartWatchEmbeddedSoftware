#ifndef __MAX30102_H__
#define __MAX30102_H__

#include <stdint.h>

void MAX30102_Init();
void MAX30102_DoSample();
void MAX30102_GetData(int32_t* hr, int32_t* spo2);

#endif
