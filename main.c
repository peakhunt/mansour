#include <stdint.h>
#include <stdio.h>
#include "pico/stdlib.h"
#include "pico/multicore.h"
#include "tusb.h"
#include "event_dispatcher.h"
#include "sys_timer.h"
#include "mainloop_timer.h"
#include "blue_led.h"
#include "ws2812_rgb.h"
#include "shell_if_usb.h"
#include "shell_commands.h"
#include "core1.h"

// XXX
// This runs on Core 0
//
static void
isr_fifo_handler()
{
  while (multicore_fifo_rvalid())
  {
    uint32_t event_id = multicore_fifo_pop_blocking();
    // Set the event bit in the Mansour Dispatcher
    event_set(1 << event_id);
  }
  multicore_fifo_clear_irq();
}

static void
multicore_setup()
{
  // 1. Setup the FIFO Interrupt on Core 0
  multicore_fifo_clear_irq();
  irq_set_exclusive_handler(SIO_IRQ_PROC0, isr_fifo_handler);
  irq_set_enabled(SIO_IRQ_PROC0, true);
}

int
main()
{
  //stdio_init_all();

  event_dispatcher_init();
  mainloop_timer_init();
  blue_led_init();
  ws2812_rgb_init();

  shell_commands_init();
  shell_if_usb_init();
  tusb_init();

  multicore_setup();
  core1_init();

  sys_timer_init();

  while(true)
  {
    event_dispatcher_dispatch();
    tud_task();
  }
}
