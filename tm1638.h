#pragma once

typedef void (*tm1638_cb_t)(uint8_t button_status);

extern void tm1638_init(void);
extern void tm1638_start_update(uint8_t *new_digits, uint8_t new_leds, tm1638_cb_t cb);
extern void tm1638_start_button_read(tm1638_cb_t cb);
extern void tm1638_get_op_step(uint8_t* op, uint8_t* step);
extern void tm1638_get_keys_raw(uint8_t d[4]);
