/*
 * this is quite unconventional TM1638 implementation.
 *
 * the primary motive here is to push the PIO to the limit
 *
 * It's PIO IRQ driven TM1638 driver.
 * Yes it may be quite inefficient in practicality
 * but an implementation with minimum jitter on mainloop has to be tested and
 * here it is.
 *
 */
#include <stdio.h>
#include "pico/stdlib.h"
#include "hardware/pio.h"
#include "hardware/irq.h"
#include "tm1638.pio.h"
#include "event_list.h"
#include "event_dispatcher.h"
#include "tm1638.h"

static PIO        pio = pio1;
static uint32_t   pio_offset;
static uint32_t   pio_sm;

#define PIN_STB 21
#define PIN_CLK 20
#define PIN_DIO 19

static const uint8_t SEG_MAP[] = 
{
  0x3F, // 0
  0x06, // 1
  0x5B, // 2
  0x4F, // 3
  0x66, // 4
  0x6D, // 5
  0x7D, // 6
  0x07, // 7
  0x7F, // 8
  0x6F, // 9
  0x77, // A
  0x7C, // b (lower case)
  0x39, // C
  0x5E, // d (lower case)
  0x79, // E
  0x71  // F
};

typedef enum
{
  TM_OP_IDLE,
  TM_OP_WRITE_ALL, // The 16-byte interleaved burst
  TM_OP_READ_KEYS  // The 4-byte key scan
} tm_op_t;

typedef struct
{
  tm_op_t  op;      // High-level state
  uint8_t  step;    // Sequence counter (0, 1, 2...)
  uint8_t  digits[8];
  uint8_t  leds;
  uint8_t  keys_raw[4];
  tm1638_cb_t callback; // The result-oriented hook
} tm1638_t;

static tm1638_t tm = 
{
  .op = TM_OP_IDLE,
  .step = 0,
  .digits = { 0, 0, 0, 0, 0, 0, 0, 0 },
  .leds = 0,
  .keys_raw = { 0, 0, 0, 0 },
  .callback = NULL,
};

static void
pio1_irq_handler(void)
{
  // XXX runs in IRQ context
  
  // 1. Clear the IRQ flag in the PIO hardware (CRITICAL)
  // If you don't do this, the CPU will get stuck in this handler
  pio_interrupt_clear(pio, 0);
  event_set(1 << DISPATCH_EVENT_TM1638_PIO_IRQ);
}

static void tm1638_pio_event_handler(uint32_t e)
{
  if (tm.op == TM_OP_WRITE_ALL)
  {
    switch (tm.step++)
    {
    case 0: // Just finished 0x40 (Cmd)
      gpio_put(PIN_STB, 1); 
      asm volatile("nop");
      gpio_put(PIN_STB, 0);
      pio_sm_put(pio, pio_sm, 0xC0); // Send Address
      break;

    case 1 ... 16:
    { // Sending the 16 interleaved bytes
      int i = (tm.step - 2) / 2;
      uint8_t val = (tm.step % 2 == 0) ? SEG_MAP[tm.digits[i]] : ((tm.leds >> i) & 1);
      pio_sm_put(pio, pio_sm, val);
      break;
    }

    case 17: // Finished data, now send Brightness/Control
      gpio_put(PIN_STB, 1);
      asm volatile("nop");
      gpio_put(PIN_STB, 0);
      pio_sm_put(pio, pio_sm, 0x8F); 
      break;

    default: // Final cleanup
      gpio_put(PIN_STB, 1);
      tm.op = TM_OP_IDLE;
      tm.callback(0);
      break;
    }
  } 
  else if (tm.op == TM_OP_READ_KEYS)
  {
    switch (tm.step++)
    {
    case 0: // Just finished 0x42 (Read Cmd)
            // Trigger first read
      pio_sm_exec(pio, pio_sm, pio_encode_jmp(pio_offset + tm1638_offset_read_8));
      pio_sm_put(pio, pio_sm, 0x00);  // send dummy to fick off read
      break;

    case 1 ... 3: // Middle bytes
      tm.keys_raw[tm.step - 2] = (uint8_t)(pio_sm_get(pio, pio_sm) >> 24);
      //pio_sm_exec(pio, pio_sm, pio_encode_jmp(pio_offset + tm1638_offset_read_8));
      pio_sm_put(pio, pio_sm, 0x00);  // send dummy to fick off read
      break;

    case 4: // Last byte
      tm.keys_raw[3] = (uint8_t)(pio_sm_get(pio, pio_sm) >> 24);
      gpio_put(PIN_STB, 1);
      uint8_t status = 0;

      for(int i = 0; i < 4; i++)
      {
        uint8_t v = tm.keys_raw[i] << i;
        status |= v;
      }

      tm.op = TM_OP_IDLE;
      tm.callback(status);
      break;
    }
  }
}

 void
tm1638_start_update(uint8_t *new_digits, uint8_t new_leds, tm1638_cb_t cb)
{
  if(tm.op != TM_OP_IDLE) return;

  // Copy data to our internal buffers
  for(int i=0; i<8; i++)
    tm.digits[i] = new_digits[i];

  tm.leds = new_leds;
  tm.callback = cb;
  tm.op = TM_OP_WRITE_ALL;
  tm.step  = 0;

  // Kick off the PIO: Pull STB low and send first command
  gpio_put(PIN_STB, 0);
  pio_sm_exec(pio, pio_sm, pio_encode_jmp(pio_offset + tm1638_offset_write_8));
  pio_sm_put(pio, pio_sm, 0x40); 
}

void
tm1638_start_button_read(tm1638_cb_t cb)
{
  if(tm.op != TM_OP_IDLE) return;

  tm.callback = cb;
  tm.op = TM_OP_READ_KEYS;
  tm.step  = 0;

  // 1. Pull STB Low to start communication
  gpio_put(PIN_STB, 0);

  // 2. Point PIO to the WRITE routine to send the "Read" command
  pio_sm_exec(pio, pio_sm, pio_encode_jmp(pio_offset + tm1638_offset_write_8));
  pio_sm_put(pio, pio_sm, 0x42); 
}

void
tm1638_init(void)
{
  // Setup Strobe pin (Standard GPIO)
  gpio_init(PIN_STB);
  gpio_set_dir(PIN_STB, GPIO_OUT);
  gpio_put(PIN_STB, 1);

  // Setup PIO
  pio_offset = pio_add_program(pio, &tm1638_program);
  pio_sm = pio_claim_unused_sm(pio, true);
  tm1638_program_init(pio, pio_sm, pio_offset, PIN_CLK, PIN_DIO);

  // PIO irq setup
  pio_set_irq0_source_enabled(pio, pis_interrupt0, true);
  irq_set_exclusive_handler(PIO1_IRQ_0, pio1_irq_handler);
  irq_set_enabled(PIO1_IRQ_0, true);

  event_register_handler(tm1638_pio_event_handler, DISPATCH_EVENT_TM1638_PIO_IRQ);
}
