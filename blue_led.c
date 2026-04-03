#include "pico/stdlib.h"
#include "mainloop_timer.h"
#include "event_dispatcher.h"
#include "event_list.h"
#include "blue_led.h"

#define LED_PIN         25

#ifdef USE_TIMER_TO_DRIVE_BLUE_LED
#define BLINK_INTERVAL  100

static SoftTimerElem  _t1;

static void
blink_callback(SoftTimerElem* te)
{
  gpio_put(LED_PIN, !gpio_get(LED_PIN));
  mainloop_timer_schedule(&_t1, BLINK_INTERVAL);
}

void
blue_led_init(void)
{
  gpio_init(LED_PIN);
  gpio_set_dir(LED_PIN, GPIO_OUT);

  soft_timer_init_elem(&_t1);
  _t1.cb = blink_callback;
  mainloop_timer_schedule(&_t1, BLINK_INTERVAL);
}
#else
static void
handle_event_from_core1(uint32_t event)
{
  gpio_put(LED_PIN, !gpio_get(LED_PIN));
}

void
blue_led_init(void)
{
  gpio_init(LED_PIN);
  gpio_set_dir(LED_PIN, GPIO_OUT);

  event_register_handler(handle_event_from_core1, DISPATCH_EVENT_CORE1_DATA);
}
#endif
