#pragma once

#include <stdint.h>
#include <stdbool.h>

typedef struct
{
  uint8_t           pin_clk;
  uint8_t           pin_dt;
  const int8_t*     table;
  volatile int32_t  count;
  uint8_t           last_status;
  uint32_t          last_time;
} encoder_t;

extern void encoder_init(encoder_t* enc, uint8_t pin_clk, uint8_t pin_dt, bool inverted);
extern int32_t get_encoder_count(encoder_t* enc);
