#include "pico/stdlib.h"
#include "hardware/gpio.h"
#include "rotary_encoder.h"

#define MAX_ENCODERS      4

// Maps exactly to your Decimal Index Tables
static const int8_t TRANS_NORMAL[]   = {0, 1, -1, 0, -1, 0, 0, 1, 1, 0, 0, -1, 0, -1, 1, 0};
static const int8_t TRANS_INVERTED[] = {0, -1, 1, 0, 1, 0, 0, -1, -1, 0, 0, 1, 0, 1, -1, 0};

static encoder_t* _encoders[MAX_ENCODERS] = 
{
  NULL,
  NULL,
  NULL,
  NULL,
};

static void
handle_encoder_irq(encoder_t* enc)
{
  uint32_t pins = gpio_get_all();
  uint8_t new_status = ((pins >> enc->pin_clk) & 1) << 1 | ((pins >> enc->pin_dt) & 1);

  if (new_status == enc->last_status) return; // No physical change, ignore IRQ noise

  int8_t move = enc->table[(enc->last_status << 2) | new_status];

  // ONLY increment if it's a valid single-step quadrature transition
  if (move == 1 || move == -1) 
  {
    enc->count += move;
    enc->last_status = new_status; // Only update state on VALID moves
  }
  // If move is 0 (illegal jump like 00->11), we DON'T update last_status.
  // This "traps" the state machine until the pins settle into a valid state.
}

static void
encoder_callback(void)
{
  for (int i = 0; i < MAX_ENCODERS; i++)
  {
    encoder_t* enc = _encoders[i];
    if (enc == NULL) continue;

    // Check if THIS specific encoder's pins fired
    uint32_t mask = gpio_get_irq_event_mask(enc->pin_clk) | gpio_get_irq_event_mask(enc->pin_dt);

    if (mask)
    {
      gpio_acknowledge_irq(enc->pin_clk, mask);
      gpio_acknowledge_irq(enc->pin_dt, mask);
      handle_encoder_irq(enc);
    }
  }
}

void
encoder_init(encoder_t* enc, uint8_t pin_clk, uint8_t pin_dt, bool inverted)
{
  // --- MBS REGISTRATION FIX ---
  int slot = -1;
  for(int i = 0; i < MAX_ENCODERS; i++) {
    if(_encoders[i] == NULL) {
      _encoders[i] = enc; // Must register the pointer to avoid NULL dereference in IRQ
      slot = i;
      break;
    }
  }
  if(slot == -1) return; // No room for more encoders

  enc->pin_clk = pin_clk;
  enc->pin_dt  = pin_dt;
  enc->table   = inverted ? TRANS_INVERTED : TRANS_NORMAL;
  enc->count   = 0;
  enc->last_time = 0;

  gpio_init_mask((1 << pin_clk) | (1 << pin_dt));
  gpio_pull_up(pin_clk);
  gpio_pull_up(pin_dt);

  enc->last_status = (gpio_get(pin_clk) << 1) | gpio_get(pin_dt);

  gpio_add_raw_irq_handler(pin_clk, encoder_callback);
  gpio_add_raw_irq_handler(pin_dt, encoder_callback);

  gpio_set_irq_enabled(pin_clk, GPIO_IRQ_EDGE_RISE | GPIO_IRQ_EDGE_FALL, true);
  gpio_set_irq_enabled(pin_dt, GPIO_IRQ_EDGE_RISE | GPIO_IRQ_EDGE_FALL, true);
  irq_set_enabled(IO_IRQ_BANK0, true);
}

int32_t
get_encoder_count(encoder_t* enc)
{
  //uint32_t status = save_and_disable_interrupts();
  int32_t val = enc->count;
  //restore_interrupts(status);
  return val;
}
