#ifndef SAMPLES_H
#define	SAMPLES_H
    
#include "sinks/displaySink.h"
#include "sinks/digio.h"
#include "sinks/dht11.h"
#include "sinks/halfduplex.h"
#include "sinks/bmp180.h"
#include "sinks/flowCounter.h"
#include "sinks/integratorSink.h"
#include "hardware/an_integrator.h"
#include "hardware/counter.h"
#include "hardware/i2c.h"
#include "hardware/max232.h"

/*
#define HAS_I2C
#define HAS_BMP180
#define HAS_DIGIO_OUT
#define HAS_DIGIO_IN
#define HAS_DHT11
#define HAS_MAX232_SOFTWARE
#define HAS_DIGITAL_COUNTER
#define HAS_ANALOG_INTEGRATOR
*/

/**
 * Init sinks
 */
void sinks_init(void);

/**
 * Poll sinks
 * Returns true if active and require polling
 */
void sinks_poll(void);

#endif	/* SAMPLES_H */

