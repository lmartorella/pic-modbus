#ifndef SAMPLES_H
#define	SAMPLES_H
    
#include "samples/bmp180.h"
#include "samples/dht11.h"
#include "samples/digio.h"
#include "samples/flowCounter.h"
#include "samples/integratorSink.h"

#undef HAS_ANALOG_INTEGRATOR
#undef HAS_BMP180
#undef HAS_DIGIO_OUT
#undef HAS_DIGIO_IN
#undef HAS_DHT11
#undef HAS_DIGITAL_COUNTER
#undef HAS_LED_BLINK

/**
 * Init samples
 */
void samples_init(void);

/**
 * Returns true if active and require polling
 */
void samples_poll(void);

#endif	/* SAMPLES_H */

