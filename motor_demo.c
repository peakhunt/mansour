#include "motor_demo.h"
#include "rotary_encoder.h"

#define ENC_CLK_PIN 10
#define ENC_DT_PIN  11

static encoder_t    _enc0;

void
motor_init(void)
{
  encoder_init(&_enc0, ENC_CLK_PIN, ENC_DT_PIN, true);
}

int32_t
motor_rotary_encoder_get(void)
{
  return get_encoder_count(&_enc0);
}
