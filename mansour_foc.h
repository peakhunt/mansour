#pragma once

#include <stdint.h>

typedef float (*foc_read_angle_t)(void);
typedef void (*foc_set_pwm_t)(uint32_t chnl, uint16_t v);

typedef struct mansour_foc_t
{
  int               pole_pairs;
  float             p_gain;
  float             i_gain;
  float             d_gain;
  float             i_error;
  float             voltage_limit;    // percentage relative to theoretical max
  float             target;
  float             last_actual;
  uint64_t          last_time;
  uint32_t          pin_phase_a;
  uint32_t          pin_phase_b;
  uint32_t          pin_phase_c;
  uint32_t          pwm_top;
  foc_read_angle_t  read_angle;
  foc_set_pwm_t     set_pwm;
} MansourFOC;

extern void mansour_foc_init(MansourFOC* foc);
extern void mansour_foc_update(MansourFOC *foc);
extern void mansour_foc_set_target_angle(MansourFOC* foc, float angle);
extern void mansour_foc_set_vlimit(MansourFOC* foc, float vlimit);
extern void mansour_foc_set_gain(MansourFOC* foc, float p, float i, float d);
