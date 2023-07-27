#ifndef _TEST_LT8920_HW_H_
#define _TEST_LT8920_HW_H_

#ifdef __cplusplus
extern "C" {
#endif

#include "configuration.h"

/**
 * Assert the reset pin for the required time
 */
void gpio_reset(_Bool asserted);

/**
 * Set register value through SPI
 */
void spi_set_reg(uint8_t reg, uint16_t val);

/**
 * Get register value through SPI
 */
uint16_t spi_get_reg(uint8_t reg);

#ifdef __cplusplus
}
#endif

#endif
