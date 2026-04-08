#pragma once

#include <stdint.h>

extern void mansour_foc_init(void);
extern float mansour_foc_get_as5600(void);
extern void mansour_foc_get_foc_info(uint16_t* hz, float* t_angle,
    float* t_actual, float* p_gain, float* d_gain, float* i_gain,
    float* v_limit);
extern void mansour_foc_set_target_angle(float angle);
extern void mansour_foc_set_vlimit(float vlimit);
extern void mansour_foc_set_gain(float p, float i, float d);
extern void mansour_foc_calc_hz(void);
