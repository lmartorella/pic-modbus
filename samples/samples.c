#include "../../src/nodes/pch.h"
#include "../../src/nodes/sinks.h"
#include "sinks/displaySink.h"
#include "sinks/digio.h"
#include "sinks/dht11.h"
#include "sinks/halfduplex.h"
#include "sinks/bmp180.h"
#include "sinks/flowCounter.h"

void sinks_init() {
#ifdef HAS_MAX232_SOFTWARE
    max232_init();
#endif

#if defined(HAS_I2C)
    i2c_init();
#endif
        
#if defined(HAS_DIGIO_IN) || defined(HAS_DIGIO_OUT)
    digio_init();
#endif

#ifdef HAS_DIGITAL_COUNTER
    dcnt_init();
#endif

#ifdef HAS_BMP180
    bmp180_init();
#endif
#ifdef HAS_DHT11
    dht11_init();
#endif
}

void sinks_poll() {
#ifdef HAS_DIGITAL_COUNTER
    if (prot_slowTimer) {
        dcnt_poll();
    }
#endif
}