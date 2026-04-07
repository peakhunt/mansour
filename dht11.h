#pragma once

extern void dht11_init(void);
extern void dht11_get(float* t, uint8_t* h, uint32_t* attempt, uint32_t* fail);
