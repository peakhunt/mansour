#include <stdint.h>
#include <stdio.h>
#include <math.h>
#include "pico/stdlib.h"
#include "hardware/pio.h"
#include "ws2812.pio.h"
#include "ws2812_rgb.h"
#include "mainloop_timer.h"

#define RGB_INTERVAL      10

static PIO      pio = pio0;
static uint32_t sm = 0;

static float angle = 0.0f;

static SoftTimerElem  _t1;

static void
rgb_callback(SoftTimerElem* te)
{
  float r_val = (sinf(angle) + 1.0f) / 2.0f;
  float g_val = (sinf(angle + 2.0f * M_PI / 3.0f) + 1.0f) / 2.0f;
  float b_val = (sinf(angle + 4.0f * M_PI / 3.0f) + 1.0f) / 2.0f;

  // Scale to 32 (dim/classy) or 255 (blinding/Taco Bibi)
  uint8_t r = (uint8_t)(r_val * 32.0f);
  uint8_t g = (uint8_t)(g_val * 32.0f);
  uint8_t b = (uint8_t)(b_val * 32.0f);

  // Pack GRB: 0xGGRRBB00
  uint32_t color = ((uint32_t)g << 24) | 
    ((uint32_t)r << 16) | 
    ((uint32_t)b << 8);

  pio_sm_put_blocking(pio, sm, color);

  angle += 0.02f; // Slowed it down for maximum "Artistic" effect
  if (angle > 2.0f * M_PI) angle -= 2.0f * M_PI;

  mainloop_timer_schedule(&_t1, RGB_INTERVAL);
}

void
ws2812_rgb_init(void)
{
  uint offset = pio_add_program(pio, &ws2812_program);
  ws2812_program_init(pio, sm, offset, 23, 800000, false);

  soft_timer_init_elem(&_t1);
  _t1.cb = rgb_callback;
  mainloop_timer_schedule(&_t1, RGB_INTERVAL);
}
