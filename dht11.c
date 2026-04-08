#include "pico/stdlib.h"
#include "hardware/pio.h"
#include "hardware/irq.h"
#include "dht11.pio.h"
#include "event_list.h"
#include "event_dispatcher.h"
#include "mainloop_timer.h"

#define DHT_PIN 15

#define DHT_READ_INTERVAL       500
#define DHT_READ_FAIL_INTERVAL  3000
#define DHT_PIN_LOW_INTERVAL    30

static SoftTimerElem  _t1;      // read interval timer
static SoftTimerElem  _t2;      // DHT11 pin low timer to start reading
static SoftTimerElem  _t_fail;

// MBS Class Data Structure
static PIO _pio;
static uint _sm;
static uint _offset;
static uint32_t _data;
static uint8_t  _checksum;
static bool _ready;
static uint32_t _attempt;
static uint32_t _fail;
static float _temperature;
static uint8_t _humidity;

////////////////////////////////////////////////////////////////////////////////
//
// DHT11 specific
//
////////////////////////////////////////////////////////////////////////////////
static inline void
dht_start_read(void)
{
  _ready = false;
  _attempt++;

  // 1. Manual Hardware Takeover
  gpio_set_dir(DHT_PIN, GPIO_OUT);
  gpio_put(DHT_PIN, 0);
}

static inline void
dht_initiaze_pio(void)
{
  // 3. Hand over to PIO
  gpio_set_dir(DHT_PIN, GPIO_IN);

  // 4. THE KICK: Push '39' to OSR to start the 40-bit hardware count
  pio_sm_put_blocking(_pio, _sm, 39);
}

static inline void
dht_handle_read_data(void)
{
  _data = pio_sm_get(_pio, _sm);
  _checksum = (uint8_t)pio_sm_get(_pio, _sm);

  _ready = true;

  uint8_t hum_h  = (_data >> 24) & 0xFF;
  uint8_t hum_l  = (_data >> 16) & 0xFF;
  uint8_t temp_h = (_data >> 8)  & 0xFF;
  uint8_t temp_l = (_data >> 0)  & 0xFF;

  // The 8-bit checksum was pushed separately in our PIO
  uint8_t checksum = _checksum;

  uint8_t calculated_sum = (hum_h + hum_l + temp_h + temp_l) & 0xFF;

  if (calculated_sum == checksum)
  {
    // no negative temperature with dht11? oh my.
    _temperature = temp_h;
    _humidity = hum_h;
  }
  else
  {
    _fail++;
  }

  gpio_set_dir(DHT_PIN, GPIO_OUT);
  gpio_put(DHT_PIN, 1);
}

static void 
dht_irq_handler()
{
  // Clear PIO IRQ 1 (second bit in the IRQ register)
  pio_interrupt_clear(_pio, 1);

  event_set(1 << DISPATCH_EVENT_DHT11_PIO_IRQ);
}

static void
dht11_pio_event_handler(uint32_t e)
{
  dht_handle_read_data();

  mainloop_timer_cancel(&_t_fail);
  mainloop_timer_schedule(&_t1, DHT_READ_INTERVAL);
}

static void
dht11_start_read_timer_callback(SoftTimerElem* te)
{
  dht_start_read();
  mainloop_timer_schedule(&_t2, DHT_PIN_LOW_INTERVAL);
  mainloop_timer_schedule(&_t_fail, DHT_READ_FAIL_INTERVAL);
}

static void
dht11_pin_low_timer_callback(SoftTimerElem* te)
{
  dht_initiaze_pio();
}

static void
dht11_read_fail_timer_callback(SoftTimerElem* te)
{
  _fail++;
  pio_sm_exec(_pio, _sm, pio_encode_jmp(_offset));
  pio_sm_clear_fifos(_pio, _sm);
  pio_sm_restart(_pio, _sm);

  dht11_start_read_timer_callback(NULL);
}

void 
dht11_init(void)
{
  _pio = pio0;
  _offset = pio_add_program(_pio, &dht11_program);
  _sm = pio_claim_unused_sm(_pio, true);
  _ready = false;

  // Use your 500kHz / 2us-per-tick initialization
  dht11_program_init(_pio, _sm, _offset, DHT_PIN);

  // Setup NVIC for PIO IRQ 1
  irq_set_exclusive_handler(PIO0_IRQ_1, dht_irq_handler);
  irq_set_enabled(PIO0_IRQ_1, true);

  // Tell the PIO hardware to route its internal IRQ 1 to the CPU
  pio_set_irq1_source_enabled(_pio, pis_interrupt1, true);

  event_register_handler(dht11_pio_event_handler, DISPATCH_EVENT_DHT11_PIO_IRQ);

  soft_timer_init_elem(&_t1);
  soft_timer_init_elem(&_t2);
  soft_timer_init_elem(&_t_fail);
  _t1.cb = dht11_start_read_timer_callback;
  _t2.cb = dht11_pin_low_timer_callback;
  _t_fail.cb = dht11_read_fail_timer_callback;

  mainloop_timer_schedule(&_t1, DHT_READ_INTERVAL);

  gpio_init(DHT_PIN);
  gpio_set_dir(DHT_PIN, GPIO_OUT);
  gpio_put(DHT_PIN, 1);
}

void
dht11_get(float* t, uint8_t* h, uint32_t* attempt, uint32_t* fail)
{
  *t = _temperature;
  *h = _humidity;
  *attempt = _attempt;
  *fail = _fail;
}
