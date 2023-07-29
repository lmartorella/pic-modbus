#ifndef _TEST_LT8920_HW_H_
#define _TEST_LT8920_HW_H_

#ifdef __cplusplus
extern "C" {
#endif

#include "configuration.h"

void hw_init(int deviceIndex);

/**
 * Assert the reset pin for the required time
 */
void gpio_reset(_Bool asserted);

/**
 * Get 8bit register value through SPI
 */
uint8_t spi_get_reg8(uint8_t reg);

/**
 * Set 8bit register value through SPI
 */
void spi_set_reg8(uint8_t reg, uint8_t val);

/**
 * Set register value through SPI, MSB first
 */
void spi_set_reg16_msb_first(uint8_t reg, uint16_t val);

/**
 * Get register value through SPI, MSB first
 */
uint16_t spi_get_reg16_msb_first(uint8_t reg);

#ifdef __cplusplus
}
#endif

#endif
