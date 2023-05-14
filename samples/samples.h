#ifndef SAMPLES_H
#define	SAMPLES_H
    
#include "sample_config.h"
#include "src/bmp180.h" 
#include "src/led_blink.h"
#include "src/hardware/i2c.h" 

#define LE_TO_BE_16(v) (((v & 0xff) << 8) + (v >> 8))

/**
 * Init samples
 */
void samples_init();

/**
 * Returns true if active and require polling
 */
void samples_poll();

#endif	/* SAMPLES_H */

