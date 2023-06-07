#ifndef __MAX30102_FIR_H_
#define __MAX30102_FIR_H_

#include <stdint.h>
#include "arm_math.h"

void max30102_fir_init(void);
void ir_max30102_fir(float *input, float *output);
void red_max30102_fir(float *input, float *output);

#endif
