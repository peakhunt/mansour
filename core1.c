#include "pico/multicore.h"
#include "event_list.h"
#include "hardware/irq.h"

__attribute__((weak)) void core1_run(void)
{
}

// This runs ONLY on Core 1
static void
core1_main()
{
  while (1)
  {
    // High-speed real-time logic here (200MHz)
    // Example: Send a "Heartbeat" to Core 0 every 1000ms
    static uint32_t last_heartbeat = 0;
    uint32_t now = time_us_32();

    core1_run();
    if (now - last_heartbeat > 1000000)
    {
      last_heartbeat = now;
      // Push an Event ID to Core 0
      multicore_fifo_push_blocking(DISPATCH_EVENT_CORE1_DATA);
    }
  }
}

void
core1_init(void)
{
  multicore_launch_core1(core1_main);
}
