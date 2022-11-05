#include <net/net.h>
#include "sinks/displaySink.h"
#include "sinks/digio.h"
#include "sinks/dht11.h"
#include "sinks/halfduplex.h"
#include "sinks/bmp180.h"
#include "sinks/flowCounter.h"
#include "sinks/integratorSink.h"
#include "hardware/an_integrator.h"
#include "hardware/i2c.h"

static __bit nil() {
    CLRWDT();
    return false;
}

// REGISTER SINKS
// Static allocation of sinks
const char* const SINK_IDS = 
    SINK_SYS_ID
#ifdef HAS_DIGIO_OUT
    DIGIO_OUT_SINK_ID
#endif
#ifdef HAS_DIGIO_IN
    DIGIO_IN_SINK_ID
#endif
#ifdef SINK_LINE_ID
    SINK_LINE_ID
#endif
#ifdef HAS_DHT11
    SINK_DHT11_ID
#endif
#ifdef HAS_MAX232_SOFTWARE
    SINK_HALFDUPLEX_ID 
#endif
#if defined(HAS_BMP180)
    SINK_BMP180_ID 
#endif
#if defined(HAS_DIGITAL_COUNTER)
    SINK_FLOW_COUNTER_ID
#endif
#if defined(HAS_ANALOG_INTEGRATOR)
    SINK_ANALOG_INTEGRATOR_ID
#endif
;

const int SINK_IDS_COUNT = 
    1
#ifdef HAS_DIGIO_OUT
    + 1
#endif
#ifdef HAS_DIGIO_IN
    + 1
#endif
#ifdef SINK_LINE_ID
    + 1
#endif
#ifdef HAS_DHT11
    + 1
#endif
#ifdef HAS_MAX232_SOFTWARE
    + 1
#endif
#if defined(HAS_BMP180)
    + 1
#endif
#if defined(HAS_DIGITAL_COUNTER)
    + 1
#endif
#if defined(HAS_ANALOG_INTEGRATOR)
    + 1
#endif
;

const SinkFunction sink_readHandlers[] = {
#ifdef _CONF_ETH_CARD
    sys_read_prim
#else
    sys_read_sec
#endif            
#ifdef HAS_DIGIO_OUT
    ,digio_out_read
#endif
#ifdef HAS_DIGIO_IN
    ,nil
#endif
#ifdef SINK_LINE_ID
    ,line_read
#endif
#ifdef HAS_DHT11
    ,nil
#endif
#ifdef HAS_MAX232_SOFTWARE
    ,halfduplex_read 
#endif
#if defined(HAS_BMP180)
    ,bmp180_sinkRead 
#endif
#if defined(HAS_DIGITAL_COUNTER)
    ,nil
#endif
#if defined(HAS_ANALOG_INTEGRATOR)
    ,nil
#endif
};

const SinkFunction sink_writeHandlers[] = {
#ifdef _CONF_ETH_CARD
    sys_write_prim
#else
    sys_write_sec
#endif            
#ifdef HAS_DIGIO_OUT
    ,digio_out_write
#endif
#ifdef HAS_DIGIO_IN
    ,digio_in_write
#endif
#ifdef SINK_LINE_ID
    ,line_write
#endif
#ifdef HAS_DHT11
    ,dht11_write
#endif
#ifdef HAS_MAX232_SOFTWARE
    ,halfduplex_write 
#endif
#if defined(HAS_BMP180)
    ,bmp180_sinkWrite 
#endif
#if defined(HAS_DIGITAL_COUNTER)
    ,flow_write
#endif
#if defined(HAS_ANALOG_INTEGRATOR)
    ,anint_sinkWrite
#endif
};

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
#ifdef HAS_ANALOG_INTEGRATOR
    anint_init();
#endif   
}

void sinks_poll() {
#ifdef HAS_DIGITAL_COUNTER
    if (prot_slowTimer) {
        dcnt_poll();
    }
#endif
#ifdef HAS_DIGIO_IN
    digio_in_poll();
#endif
#ifdef HAS_ANALOG_INTEGRATOR
    anint_poll();
#endif   
}