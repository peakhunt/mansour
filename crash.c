#include <stdio.h>
#include <stdarg.h>
#include "pico/stdlib.h"
#include "hardware/sync.h"
#include "crash.h"

#define CRASH_MSG_BUFFER_SIZE     128

static const char*  _file;
static int          _line;
static char         _crash_msg[CRASH_MSG_BUFFER_SIZE];

void __attribute__((noreturn)) 
generic_crash(const char *file, int line, const char *fmt, ...)
{
  // 1. Immediately disable all interrupts to stop core loops/PIO
  save_and_disable_interrupts();

  // 2. Put critical hardware into a safe state
  // Add other critical hardware safety puts here (e.g., motor PWM to 0)

  // 3. save crash report in mem. we don't have console 
  // later, we might be able to see this with debugger
  _file = file;
  _line = line;

  va_list args;
  va_start(args, fmt);
  vsnprintf(_crash_msg, CRASH_MSG_BUFFER_SIZE, fmt, args);
  va_end(args);

  while (true) {
    busy_wait_ms(100); // Use busy_wait because interrupts are disabled
  }
}
