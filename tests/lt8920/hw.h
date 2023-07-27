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
 * Set register value through SPI, MSB first
 */
void spi_set_reg_msb_first(uint8_t reg, uint16_t val);

/**
 * Get register value through SPI, MSB first
 */
uint16_t spi_get_reg_msb_first(uint8_t reg);

#ifdef __cplusplus
}
#endif

#endif
