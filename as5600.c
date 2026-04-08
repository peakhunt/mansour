#include "hardware/i2c.h"
#include "pico/stdlib.h"
#include <stdint.h>

#define AS5600_ADDR 0x36
#define ANGLE_REG_HI 0x0E

#define AS5600_I2C_SCL      5
#define AS5600_I2C_SDA      4

uint16_t
as5600_read(void)
{
  uint8_t reg = ANGLE_REG_HI;
  uint8_t buffer[2];

  // High-speed burst read
  i2c_write_blocking(i2c0, AS5600_ADDR, &reg, 1, true);
  i2c_read_blocking(i2c0, AS5600_ADDR, buffer, 2, false);

  return ((uint16_t)buffer[0] << 8) | buffer[1];
}

void
as5600_init(void)
{
  i2c_init(i2c0, 1000 * 1000); 
  gpio_set_function(AS5600_I2C_SDA, GPIO_FUNC_I2C); // SDA
  gpio_set_function(AS5600_I2C_SCL, GPIO_FUNC_I2C); // SCL
  gpio_pull_up(AS5600_I2C_SDA);
  gpio_pull_up(AS5600_I2C_SCL);
}
