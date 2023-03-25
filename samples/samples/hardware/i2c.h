#ifndef _I2C_INCLUDE_
#define _I2C_INCLUDE_

#include "./sample_config.h"

void i2c_init();
// Returns TRUE if device is IDLE
__bit i2c_poll();

void i2c_sendReceive7(uint8_t addrRw, uint8_t size, uint8_t* buf);

#endif
