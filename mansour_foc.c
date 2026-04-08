#include <stdio.h>
#include <math.h>
#include "hardware/pwm.h"
#include "hardware/clocks.h"
#include "pico/stdlib.h"
#include "mansour_foc.h"
#include "as5600.h"
#include "mainloop_timer.h"

// FOC Constants
#define PI_2           1.5707963f
#define TWO_PI_3       2.0943951f
#define FOUR_PI_3      4.1887902f
#define DEG_TO_RAD     0.01745329f

#define TIMER_FOC_UPDATE_INTERVAL       1
#define TIMER_LOOP_COUNT                1000

#define PIN_PHASE_A    0
#define PIN_PHASE_B    1
#define PIN_PHASE_C    2

// Target: 20kHz Center-Aligned (Phase-Correct) SPWM
// F_sys = 200MHz
// F_pwm = F_sys / (2 * TOP * DIV)
// 20,000 = 200,000,000 / (2 * 5000 * 1)
#define PWM_TOP         5000 

typedef struct
{
  int       pole_pairs;
  float     p_gain;
  float     i_gain;
  float     d_gain;
  float     i_error;
  float     voltage_limit;    // percentage relative to theoretical max
  float     target;
  float     last_actual;
  uint64_t  last_time;
} MansourFOC;

static void mansour_foc_update(MansourFOC *foc);

static SoftTimerElem    _t_foc_update;
static SoftTimerElem    _t_loop_count;

static uint16_t         _as5600_angle;
static uint16_t         _hz;
static uint16_t         _count = 0;

static MansourFOC _motor;

static void
timer_loop_count(SoftTimerElem* e)
{
  _hz = _count;
  _count = 0;
  //mainloop_timer_schedule(&_t_loop_count, TIMER_LOOP_COUNT);
}

static void
timer_foc_update(SoftTimerElem* e)
{
  mansour_foc_update(&_motor);
  mainloop_timer_schedule(&_t_foc_update, TIMER_FOC_UPDATE_INTERVAL);
  _count++;
}

static void
mansour_foc_pwm_init(void)
{
  // 1. Hardware Pin Muxing
  gpio_set_function(PIN_PHASE_A, GPIO_FUNC_PWM);
  gpio_set_function(PIN_PHASE_B, GPIO_FUNC_PWM);
  gpio_set_function(PIN_PHASE_C, GPIO_FUNC_PWM);

  // 2. Identify Slices
  uint slice_0 = pwm_gpio_to_slice_num(PIN_PHASE_A); // Handles GP0 & GP1
  uint slice_1 = pwm_gpio_to_slice_num(PIN_PHASE_C); // Handles GP2

  // 3. Configure Slice 0 (Phases A & B)
  pwm_config config0 = pwm_get_default_config();
  pwm_config_set_phase_correct(&config0, true);
  pwm_config_set_wrap(&config0, PWM_TOP);
  pwm_config_set_clkdiv(&config0, 1.0f); // No division for max resolution
  pwm_init(slice_0, &config0, false);

  // 4. Configure Slice 1 (Phase C)
  pwm_config config1 = pwm_get_default_config();
  pwm_config_set_phase_correct(&config1, true);
  pwm_config_set_wrap(&config1, PWM_TOP);
  pwm_config_set_clkdiv(&config1, 1.0f);
  pwm_init(slice_1, &config1, false);

  // 5. Center-Align Reset & Synchronous Enable
  // We clear the counters to 0 first to ensure they start in perfect phase
  pwm_set_counter(slice_0, 0);
  pwm_set_counter(slice_1, 0);

  // Start both slices at the exact same clock cycle using the enable mask
  pwm_set_mask_enabled((1 << slice_0) | (1 << slice_1));

  // 6. Set Initial Duty to 0 (Safe State)
  pwm_set_gpio_level(PIN_PHASE_A, 0);
  pwm_set_gpio_level(PIN_PHASE_B, 0);
  pwm_set_gpio_level(PIN_PHASE_C, 0);
}

#if 0
static void
mansour_foc_update(MansourFOC *foc)
{
  uint64_t now = time_us_64();
  float dt = (float)(now - foc->last_time) * 1e-6f;
  if (dt <= 0.0001f) return; // Cap loop speed
  foc->last_time = now;

  uint16_t raw = as5600_read();
  _as5600_angle = raw;
  float actual = ((float)raw * 360.0f) / 4096.0f;

  // Control Logic
  float norm_target = fmodf(foc->target, 360.0f);
  if (norm_target < 0.0f) norm_target += 360.0f;

  float error = actual - norm_target;
  if (error > 180.0f) error -= 360.0f;
  if (error < -180.0f) error += 360.0f;

  float velocity = (actual - foc->last_actual) / dt;
  float voltage = (error * foc->p_gain) - (velocity * foc->d_gain);

  // Clamp
  if (voltage > foc->voltage_limit) voltage = foc->voltage_limit;
  if (voltage < -foc->voltage_limit) voltage = -foc->voltage_limit;

  // Transform
  float rad_actual = actual * DEG_TO_RAD;
  float elec_angle = (rad_actual * (float)foc->pole_pairs) + ((voltage >= 0.0f) ? PI_2 : -PI_2);

  float amp_scaled = (fabsf(voltage) / 100.0f) * (PWM_TOP / 2.0f);   // hkim
  float offset = PWM_TOP / 2.0f; 

  // Update Hardware
  pwm_set_gpio_level(PIN_PHASE_A, (uint16_t)(offset + (sinf(elec_angle) * amp_scaled)));
  pwm_set_gpio_level(PIN_PHASE_B, (uint16_t)(offset + (sinf(elec_angle + TWO_PI_3) * amp_scaled)));
  pwm_set_gpio_level(PIN_PHASE_C, (uint16_t)(offset + (sinf(elec_angle + FOUR_PI_3) * amp_scaled)));

  foc->last_actual = actual;
}
#else
static void
mansour_foc_update(MansourFOC *foc)
{
  uint64_t now = time_us_64();
  float dt = (float)(now - foc->last_time) * 1e-6f;
  if (dt <= 0.0001f) return; 
  foc->last_time = now;

  uint16_t raw = as5600_read();
  _as5600_angle = raw;
  float actual = ((float)raw * 360.0f) / 4096.0f;

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

  float amp_scaled = (fabsf(voltage) / 100.0f) * (PWM_TOP / 2.0f);
  float offset = PWM_TOP / 2.0f; 

  // Update Hardware
  pwm_set_gpio_level(PIN_PHASE_A, (uint16_t)(offset + (sinf(elec_angle) * amp_scaled)));
  pwm_set_gpio_level(PIN_PHASE_B, (uint16_t)(offset + (sinf(elec_angle + TWO_PI_3) * amp_scaled)));
  pwm_set_gpio_level(PIN_PHASE_C, (uint16_t)(offset + (sinf(elec_angle + FOUR_PI_3) * amp_scaled)));

  foc->last_actual = actual;
}
#endif

float
mansour_foc_get_as5600(void)
{
  return (_as5600_angle / 4096.0f)*360.0f;
}

void
mansour_foc_get_foc_info(uint16_t* hz, float* t_angle,
    float* t_actual, float* p_gain, float* d_gain, float* i_gain,
    float* v_limit)
{
  *hz = _hz;
  *t_angle = _motor.target;
  *t_actual = _motor.last_actual;
  *p_gain = _motor.p_gain;
  *i_gain = _motor.i_gain;
  *d_gain = _motor.d_gain;
  *v_limit = _motor.voltage_limit;
}

void
mansour_foc_set_target_angle(float angle)
{
  _motor.target = angle;
  _motor.i_error = 0.0f;
}

void
mansour_foc_set_vlimit(float vlimit)
{
  _motor.voltage_limit = vlimit;
}

void
mansour_foc_set_gain(float p, float i, float d)
{
  _motor.p_gain = p;
  _motor.i_gain = i;
  _motor.d_gain = d;
  _motor.i_error = 0.0f;
}

void
mansour_foc_calc_hz(void)
{
  timer_loop_count(NULL);
}

void
mansour_foc_init(void)
{
  as5600_init();
  mansour_foc_pwm_init();

  soft_timer_init_elem(&_t_foc_update);
  _t_foc_update.cb = timer_foc_update;
  soft_timer_init_elem(&_t_loop_count);
  _t_loop_count.cb = timer_loop_count;

  _motor = 
  (MansourFOC)
  {
    .pole_pairs = 7,
    .p_gain = 8.0f,
    .i_gain = 0.5f,
    .d_gain = 0.01f,
    .i_error = 0.0,
    .voltage_limit = 10.0f,
    .target = 0.0f,
    .last_time = time_us_64()
  };

  uint16_t start_raw = as5600_read();
  _as5600_angle = start_raw;
  _motor.last_actual = ((float)start_raw * 360.0f) / 4096.0f;
  //_motor.target = _motor.last_actual;

  mainloop_timer_schedule(&_t_foc_update, TIMER_FOC_UPDATE_INTERVAL);
  //mainloop_timer_schedule(&_t_loop_count, TIMER_LOOP_COUNT);
}
