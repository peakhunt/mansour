#include "pico/stdlib.h"
#include "mainloop_timer.h"
#include "tm1638.h"
#include "tm1638_demo.h"

#define READ_UPDATE_INTERVAL      50
#define SEG_UPDATE_INTERVAL       100
#define WAIT_INTERVAL             10

typedef enum
{
  STATE_IDLE,
  STATE_WRITE_ALL,
} tm1638_demo_state_t;

static SoftTimerElem          _t1;
static SoftTimerElem          _t2;
static tm1638_demo_state_t    _state = STATE_IDLE;

static uint8_t digits[8] = { 0, 0, 0, 0, 0, 0, 0, 0 };
static uint8_t leds = 0;
static uint32_t hex_counter = 0;

static uint8_t _prev_btn_state = 0;    // to edge detect

static void
tm1638_button_read_complete(uint8_t btn)
{
  uint8_t bit;
  _state = STATE_WRITE_ALL;

  for(int i = 0; i < 8; i++)
  {
    bit = (0x01 << i);

    // detect rising edge
    if((_prev_btn_state & bit) == 0 && (btn & bit))
    {
      // button pressed
      if(leds & bit)
      {
        // turn off
        leds &= ~(bit);
      }
      else
      {
        // turn on
        leds |= (bit);
      }
    }
  }
  _prev_btn_state = btn;

  mainloop_timer_schedule(&_t1, WAIT_INTERVAL);
}

static void
tm1638_write_all_complete(uint8_t btn)
{
  _state = STATE_IDLE;
  mainloop_timer_schedule(&_t1, READ_UPDATE_INTERVAL);
}

static void
tm1638_demo_timer_callback(SoftTimerElem* te)
{
  switch(_state)
  {
  case STATE_IDLE:
    tm1638_start_button_read(tm1638_button_read_complete);
    break;

  case STATE_WRITE_ALL:
    tm1638_start_update(digits, leds, tm1638_write_all_complete);
    break;
  }
}

static void
tm1638_init_update_complete(uint8_t btn)
{
  //tm1638_start_button_read(tm1638_button_read_complete);
  mainloop_timer_schedule(&_t1, WAIT_INTERVAL);
}

static void
tm1638_demo_update_callback(SoftTimerElem* te)
{
  hex_counter++;
  for (int i = 0; i < 8; i++)
  {
    // Unpack from right to left (LSB at index 7 or 0 depending on your preference)
    // Usually, index 0 is the leftmost digit on these modules
    digits[7 - i] = (hex_counter >> (i * 4)) & 0x0F;
  }
  mainloop_timer_schedule(&_t2, SEG_UPDATE_INTERVAL);
}

void
tm1638_demo_init(void)
{
  tm1638_init();

  soft_timer_init_elem(&_t1);
  _t1.cb = tm1638_demo_timer_callback;

  soft_timer_init_elem(&_t2);
  _t2.cb = tm1638_demo_update_callback;

  tm1638_start_update(digits, leds, tm1638_init_update_complete);
  mainloop_timer_schedule(&_t2, SEG_UPDATE_INTERVAL);
}
