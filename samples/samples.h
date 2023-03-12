#ifndef SAMPLES_H
#define	SAMPLES_H
    
#include "sample_config.h"
#include "samples/led_blink.h"


/**
 * Init samples
 */
void samples_init();

/**
 * Returns true if active and require polling
 */
void samples_poll();

#endif	/* SAMPLES_H */

