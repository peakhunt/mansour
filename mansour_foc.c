#include <stdio.h>
#include <math.h>
#include "hardware/pwm.h"
#include "hardware/clocks.h"
#include "pico/stdlib.h"
#include "mansour_foc.h"

// FOC Constants
#define PI_2           1.5707963f
#define TWO_PI_3       2.0943951f
#define FOUR_PI_3      4.1887902f
#define DEG_TO_RAD     0.01745329f

void
mansour_foc_update(MansourFOC *foc)
{
  uint64_t now = time_us_64();
  float dt = (float)(now - foc->last_time) * 1e-6f;
  if (dt <= 0.0001f) return; 
  foc->last_time = now;

  float actual = foc->read_angle();

  // Control Logic
  float norm_target = fmodf(foc->target, 360.0f);
  if (norm_target < 0.0f) norm_target += 360.0f;

  float error = actual - norm_target;
  if (error > 180.0f) error -= 360.0f;
  if (error < -180.0f) error += 360.0f;

  // 1. Calculate Integral with Windup Protection
  // Only let the I-term work if the error is small (the "finishing" move)
  if (fabsf(error) < 5.0f) {
    foc->i_error += error * dt;
  } else {
    foc->i_error = 0; // Clear it if we are moving fast
  }
  //foc->i_error += error * dt;
  // Limit the "power" the I-term can have (e.g., max 20% of total voltage)
  if (foc->i_error > 100.0f) foc->i_error = 100.0f;
  if (foc->i_error < -100.0f) foc->i_error = -100.0f;

  float velocity = (actual - foc->last_actual) / dt;
  
  // 2. Full PID: added (foc->i_error * foc->i_gain)
  float voltage = (error * foc->p_gain) + (foc->i_error * foc->i_gain) - (velocity * foc->d_gain);

  // Clamp
  if (voltage > foc->voltage_limit) voltage = foc->voltage_limit;
  if (voltage < -foc->voltage_limit) voltage = -foc->voltage_limit;

  // Transform
  float rad_actual = actual * DEG_TO_RAD;
  float elec_angle = (rad_actual * (float)foc->pole_pairs) + ((voltage >= 0.0f) ? PI_2 : -PI_2);

  float amp_scaled = (fabsf(voltage) / 100.0f) * (foc->pwm_top / 2.0f);
  float offset = foc->pwm_top / 2.0f; 

  // Update Hardware
  foc->set_pwm(foc->pin_phase_a, (uint16_t)(offset + (sinf(elec_angle) * amp_scaled)));
  foc->set_pwm(foc->pin_phase_b, (uint16_t)(offset + (sinf(elec_angle + TWO_PI_3) * amp_scaled)));
  foc->set_pwm(foc->pin_phase_c, (uint16_t)(offset + (sinf(elec_angle + FOUR_PI_3) * amp_scaled)));

  foc->last_actual = actual;
}

void
mansour_foc_set_target_angle(MansourFOC* foc, float angle)
{
  foc->target = angle;
  foc->i_error = 0.0f;
}

void
mansour_foc_set_vlimit(MansourFOC* foc, float vlimit)
{
  foc->voltage_limit = vlimit;
}

void
mansour_foc_set_gain(MansourFOC* foc, float p, float i, float d)
{
  foc->p_gain = p;
  foc->i_gain = i;
  foc->d_gain = d;
  foc->i_error = 0.0f;
}

void
mansour_foc_init(MansourFOC* foc)
{
  foc->last_time = time_us_64();
  foc->last_actual = foc->read_angle();
}
