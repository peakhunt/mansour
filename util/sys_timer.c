#include "hardware/structs/systick.h"
#include "hardware/exception.h"
#include "hardware/clocks.h"
#include "sys_timer.h"
#include "event_dispatcher.h"
#include "event_list.h"

volatile uint32_t     __uptime = 0;
volatile uint32_t     __millis = 0;

static void
isr_systick(void)
{
  static uint32_t counter = 0;

  __millis++;
  counter++;

  if(counter >= 1000)
  {
    __uptime++;
    counter = 0;
  }
  event_set(1 << DISPATCH_EVENT_TIMER_TICK);
}

void
sys_timer_init(void)
{
  exception_set_exclusive_handler(SYSTICK_EXCEPTION, isr_systick);

  uint32_t cpu_hz = clock_get_hz(clk_sys);

  systick_hw->rvr = (cpu_hz/1000) - 1;  // 1ms
  systick_hw->cvr = 0;    // 3. Reset Current Value
  systick_hw->csr = 0x7;  // 4. CSR: Enable (bit 0), Enable Interrupt (bit 1), Source = CPU Clock (bit 2)
}

uint32_t
sys_millis(void)
{
  return __millis;
}
