#ifndef _I2C_INCLUDE_
#define _I2C_INCLUDE_

#ifdef HAS_I2C

void i2c_init();
// Returns TRUE if device is IDLE
 bit i2c_poll();

void i2c_sendReceive7(BYTE addrRw, BYTE size, BYTE* buf);

#endif
#endif
