#include <stdint.h>
#include <stdio.h>
#include <math.h>
#include "pico/stdlib.h"
#include "hardware/pio.h"
#include "hardware/dma.h"
#include "hardware/irq.h"
#include "ws2812.pio.h"
#include "ws2812_rgb.h"
#include "mainloop_timer.h"

#define RGB_INTERVAL      10

#define NUM_LEDS          16
static uint32_t led_framebuffer[NUM_LEDS];
static int dma_chan;

static PIO      pio = pio0;
static uint32_t sm = 0;
static uint32_t sm2 = 1;

static float angle = 0.0f;
static float angle2 = 0.0f;

static SoftTimerElem  _t1;
static SoftTimerElem  _t2;

static 
void dma_handler()
{
  // 1. Clear the interrupt flag so it doesn't loop forever
  dma_hw->ints0 = 1u << dma_chan;
}

static void
setup_dma(PIO t_pio, uint t_sm)
{
  dma_chan = dma_claim_unused_channel(true);
  dma_channel_config c = dma_channel_get_default_config(dma_chan);

  channel_config_set_transfer_data_size(&c, DMA_SIZE_32);
  channel_config_set_read_increment(&c, true);
  channel_config_set_write_increment(&c, false);
  channel_config_set_dreq(&c, pio_get_dreq(t_pio, t_sm, true));

  dma_channel_configure(
      dma_chan, &c,
      &t_pio->txf[t_sm], 
      led_framebuffer, 
      NUM_LEDS, 
      false 
      );

  // --- IRQ SETUP ---
  // Tell DMA to raise IRQ 0 when finished
  dma_channel_set_irq0_enabled(dma_chan, true);

  // Map the handler function to the DMA IRQ
  irq_set_exclusive_handler(DMA_IRQ_0, dma_handler);
  irq_set_enabled(DMA_IRQ_0, true);

  // Kick off the first transfer
  dma_handler(); 
}

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
    ((uint32_t)b << 0);
  

  pio_sm_put_blocking(pio, sm, color);

  angle += 0.02f; // Slowed it down for maximum "Artistic" effect
  if (angle > 2.0f * M_PI) angle -= 2.0f * M_PI;

  mainloop_timer_schedule(&_t1, RGB_INTERVAL);
}

static void
update_mbs_meteor_buffer()
{
  static float position = 0.0f;
  const float velocity = 0.22f;

  position += velocity; 
  if (position >= 16.0f)
    position -= 16.0f;

  for (int i = 0; i < 16; i++)
  {
    // 1. Calculate signed distance (i - position)
    float diff = (float)i - position;

    // Handle wrap-around for the 16-LED ring
    if (diff > 8.0f) diff -= 16.0f;
    if (diff < -8.0f) diff += 16.0f;

    // 2. DIRECTIONAL LOGIC: 
    // If diff is negative, the pixel is BEHIND the head (The Tail).
    // If diff is positive, it's IN FRONT of the head (Darkness).

    uint8_t r = 0, g = 0, b = 0;
    float dist = fabsf(diff);

    if (diff > 0.1f)
    {
      // Instant cutoff in front of the meteor
      r = 0; g = 0; b = 0;
    } 
    else if (dist < 0.5f)
    {
      // THE HEAD: White-Hot impact point
      r = 20; g = 50; b = 255;
    } 
    else if (diff < 0)
    {
      // THE TAIL: Only exists where diff is negative (behind the head)
      if (dist < 1.5f)
      {
        // Molten Core
        float fade = 1.0f - ((dist - 0.5f) / 1.0f);
        r = 20; g = (uint8_t)(50 * fade); b = (uint8_t)(255 * fade);
      }
      else if (dist < 7.0f)
      {
        // Long, Deep Red Plasma Trail
        float tail_fade = powf(1.0f - ((dist - 1.5f) / 5.5f), 3.0f);
        r = (uint8_t)(200.0f * tail_fade);
      }
    }

    led_framebuffer[i] = ((uint32_t)g << 24) | ((uint32_t)r << 16) | ((uint32_t)b << 8);
  }
}

static void
mbs_callback(SoftTimerElem* te)
{
  update_mbs_meteor_buffer();

  // Trigger the DMA to ship the entire buffer to the ring
  dma_channel_set_read_addr(dma_chan, led_framebuffer, true);

  mainloop_timer_schedule(&_t2, 20);
}

void
ws2812_rgb_init(void)
{
  uint offset = pio_add_program(pio, &ws2812_program);
  sm = pio_claim_unused_sm(pio, true);
  sm2 = pio_claim_unused_sm(pio, true);
  ws2812_program_init(pio, sm, offset, 23, 800000, false);
  ws2812_program_init(pio, sm2, offset, 22, 800000, false);

  soft_timer_init_elem(&_t1);
  _t1.cb = rgb_callback;

  soft_timer_init_elem(&_t2);
  _t2.cb = mbs_callback;

  setup_dma(pio, sm2);

  mainloop_timer_schedule(&_t1, RGB_INTERVAL);
  mainloop_timer_schedule(&_t2, RGB_INTERVAL);
}
