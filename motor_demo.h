#pragma once

#include <stdint.h>

extern void motor_init(void);
extern int32_t motor_rotary_encoder_get(void);
extern void motor_get_foc_info(uint32_t* hz, float* t_angle,
    float* t_actual, float* p_gain, float* d_gain, float* i_gain,
    float* v_limit);
extern void motor_foc_set_target_angle(float angle);
extern void motor_foc_set_vlimit(float vlimit);
extern void motor_foc_set_gain(float p, float i, float d);
extern void motor_foc_calc_hz(void);
