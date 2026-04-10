#include <math.h>
#include "hardware/pwm.h"
#include "hardware/timer.h"
#include "pico/stdlib.h"
#include "motor_demo.h"
#include "mansour_foc.h"
#include "as5600.h"
#include "rotary_encoder.h"
#include "mainloop_timer.h"
#include "event_list.h"
#include "event_dispatcher.h"

#define ENC_CLK_PIN 10
#define ENC_DT_PIN  11

#define PIN_PHASE_A    0
#define PIN_PHASE_B    1
#define PIN_PHASE_C    2

// Target: 20kHz Center-Aligned (Phase-Correct) SPWM
// F_sys = 200MHz
// F_pwm = F_sys / (2 * TOP * DIV)
// 20,000 = 200,000,000 / (2 * 5000 * 1)
#define PWM_TOP         5000 

#define TIMER_FOC_UPDATE_INTERVAL       1
#define TIMER_LOOP_COUNT                1000

// 4kHz = 250 microseconds
#define TIMER_REPEATING_FOC_UPDATE_US 125

//
// enable only one of these or it will be screwed up
// 
// if you enable IRQ based update source, don't forget
// to modify mansource_foc.h
//
#define MOTOR_FOC_UPDATE_SOURCE_MAINLOOP_TIMER      (1)
#define MOTOR_FOC_UPDATE_SOURCE_REPEATING_TIMER     (0)
#define MOTOR_FOC_UPDATE_SOURCE_PWM_IRQ             (0)
#define MOTOR_FOC_UPDATE_CORE1                      (0)

static float foc_read_angle(void);
static void foc_set_pwm(uint32_t chnl, uint16_t v);

static encoder_t        _enc0;
static MansourFOC       _foc =
{
  .pole_pairs     = 7,
  .p_gain         = 8.0f,
  .i_gain         = 0.5f,
  .d_gain         = 0.01f,
  .i_error        = 0.0,
  .voltage_limit  = 10.0f,
  .target         = 0.0f,
  .last_time      = 0,
  .pin_phase_a    = PIN_PHASE_A,
  .pin_phase_b    = PIN_PHASE_B,
  .pin_phase_c    = PIN_PHASE_C,
  .pwm_top        = PWM_TOP,
  .read_angle     = foc_read_angle,
  .set_pwm        = foc_set_pwm,
};

static SoftTimerElem    _t_foc_update;
static SoftTimerElem    _t_loop_count;
static struct repeating_timer _timer;

static uint32_t                 _hz;
#if (MOTOR_FOC_UPDATE_SOURCE_PWM_IRQ == 1) ||           \
    (MOTOR_FOC_UPDATE_SOURCE_REPEATING_TIMER == 1) ||   \
    (MOTOR_FOC_UPDATE_CORE1 == 1)
static volatile uint32_t        _count = 0;
#else
static uint32_t                 _count = 0;
#endif
static int32_t                  _enc_val = 0;

static void
timer_loop_count(SoftTimerElem* e)
{
  _hz = _count;
  _count = 0;
  //mainloop_timer_schedule(&_t_loop_count, TIMER_LOOP_COUNT);
}

#if MOTOR_FOC_UPDATE_SOURCE_REPEATING_TIMER == 1
static bool
foc_update_handler(struct repeating_timer *t)
{
  mansour_foc_update(&_foc);
  _count++;
  return true; // Keep the timer repeating
}
#elif MOTOR_FOC_UPDATE_SOURCE_PWM_IRQ == 1
static void
foc_update_handler(void)
{
  pwm_clear_irq(0);
  mansour_foc_update(&_foc);
  _count++;
}
#elif MOTOR_FOC_UPDATE_SOURCE_MAINLOOP_TIMER == 1
static void
foc_update_handler(SoftTimerElem* e)
{
  mansour_foc_update(&_foc);
  mainloop_timer_schedule(&_t_foc_update, TIMER_FOC_UPDATE_INTERVAL);
  _count++;
}
#elif MOTOR_FOC_UPDATE_CORE1 == 1
void core1_run(void)
{
  mansour_foc_update(&_foc);
  _count++;
}
#endif

static float
foc_read_angle(void)
{
  uint16_t raw = as5600_read();
  return ((float)raw * 360.0f) / 4096.0f;
} 

static void
foc_set_pwm(uint32_t chnl, uint16_t v)
{
  pwm_set_gpio_level(chnl, v);
}

static void
handle_renc_change(uint32_t event)
{
  int32_t rv = get_encoder_count(&_enc0);
  int32_t diff = rv - _enc_val;

  _enc_val = rv;

  float move_angle = 10 * diff;
  float angle = _foc.target + move_angle;

  if(angle < 0)
  {
    angle += 360;
  }

  angle = fmodf(angle, 360);

  mansour_foc_set_target_angle(&_foc, angle);
}

static void
foc_pwm_init(void)
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

static void
motor_setup_foc_update_source(void)
{
#if MOTOR_FOC_UPDATE_SOURCE_MAINLOOP_TIMER == 1
  soft_timer_init_elem(&_t_foc_update);
  _t_foc_update.cb = foc_update_handler;
  soft_timer_init_elem(&_t_loop_count);
  _t_loop_count.cb = timer_loop_count;

  mainloop_timer_schedule(&_t_foc_update, TIMER_FOC_UPDATE_INTERVAL);
  //mainloop_timer_schedule(&_t_loop_count, TIMER_LOOP_COUNT);
#elif MOTOR_FOC_UPDATE_SOURCE_REPEATING_TIMER == 1
  add_repeating_timer_us(-TIMER_REPEATING_FOC_UPDATE_US, foc_update_handler, NULL, &_timer);
#elif MOTOR_FOC_UPDATE_SOURCE_PWM_IRQ == 1
  uint slice_0 = pwm_gpio_to_slice_num(PIN_PHASE_A); // Handles GP0 & GP1
  pwm_set_irq_enabled(slice_0, true);
  irq_set_exclusive_handler(PWM_IRQ_WRAP, foc_update_handler);
  irq_set_enabled(PWM_IRQ_WRAP, true);
#endif
}

void
motor_init(void)
{
  encoder_init(&_enc0, ENC_CLK_PIN, ENC_DT_PIN, true);
  foc_pwm_init();
  as5600_init();

  mansour_foc_init(&_foc);

  event_register_handler(handle_renc_change, DISPATCH_EVENT_RENC_CHANGE);

  motor_setup_foc_update_source();
}

void
motor_foc_calc_hz(void)
{
  timer_loop_count(NULL);
}

int32_t
motor_rotary_encoder_get(void)
{
  return get_encoder_count(&_enc0);
}

void
motor_get_foc_info(uint32_t* hz, float* t_angle,
    float* t_actual, float* p_gain, float* d_gain, float* i_gain,
    float* v_limit)
{
  *hz = _hz;
  *t_angle = _foc.target;
  *t_actual = _foc.last_actual;
  *p_gain = _foc.p_gain;
  *i_gain = _foc.i_gain;
  *d_gain = _foc.d_gain;
  *v_limit = _foc.voltage_limit;
}

void
motor_foc_set_target_angle(float angle)
{
  mansour_foc_set_target_angle(&_foc, angle);
}

void
motor_foc_set_vlimit(float vlimit)
{
  mansour_foc_set_vlimit(&_foc, vlimit);
}

void
motor_foc_set_gain(float p, float i, float d)
{
  mansour_foc_set_gain(&_foc, p, i, d);
}
