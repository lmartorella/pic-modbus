#include <net/net.h>
#include "./samples.h"

// REGISTER SINKS
// Static allocation of sinks
const FunctionDefinition bus_cl_functions[] = {
    {
        AUTOCONF_NODE_STATUS_ID,
        (void (*)(void*))autoconf_readNodeStatus,
        sizeof(AUTOCONF_NODE_STATUS),
        NULL, 
        0
    }
};
const uint8_t bus_cl_function_count = 1;

/*
#ifdef HAS_DIGIO_OUT
    DIGIO_OUT_SINK_ID
#endif
#ifdef HAS_DIGIO_IN
    DIGIO_IN_SINK_ID
#endif
#ifdef HAS_CM1602
    SINK_LINE_ID
#endif
#ifdef HAS_DHT11
    SINK_DHT11_ID
#endif
#ifdef HAS_MAX232_SOFTWARE
    SINK_HALFDUPLEX_ID 
#endif
#ifdef HAS_BMP180
    SINK_BMP180_ID 
#endif
#ifdef HAS_DIGITAL_COUNTER
    SINK_FLOW_COUNTER_ID
#endif
#ifdef HAS_ANALOG_INTEGRATOR
    SINK_ANALOG_INTEGRATOR_ID
#endif
;

const uint16_t SINK_IDS_COUNT = 
    1
#ifdef HAS_DIGIO_OUT
    + 1
#endif
#ifdef HAS_DIGIO_IN
    + 1
#endif
#ifdef HAS_CM1602
    + 1
#endif
#ifdef HAS_DHT11
    + 1
#endif
#ifdef HAS_MAX232_SOFTWARE
    + 1
#endif
#ifdef HAS_BMP180
    + 1
#endif
#ifdef HAS_DIGITAL_COUNTER
    + 1
#endif
#ifdef HAS_ANALOG_INTEGRATOR
    + 1
#endif
;

const SinkFunction sink_readHandlers[] = {
    sys_read
#ifdef HAS_DIGIO_OUT
    ,digio_out_read
#endif
#ifdef HAS_DIGIO_IN
    ,nil
#endif
#ifdef HAS_CM1602
    ,line_read
#endif
#ifdef HAS_DHT11
    ,nil
#endif
#ifdef HAS_MAX232_SOFTWARE
    ,halfduplex_read 
#endif
#ifdef HAS_BMP180
    ,bmp180_sinkRead 
#endif
#ifdef HAS_DIGITAL_COUNTER
    ,nil
#endif
#ifdef HAS_ANALOG_INTEGRATOR
    ,nil
#endif
};

const SinkFunction sink_writeHandlers[] = {
    sys_write
#ifdef HAS_DIGIO_OUT
    ,digio_out_write
#endif
#ifdef HAS_DIGIO_IN
    ,digio_in_write
#endif
#ifdef HAS_CM1602
    ,line_write
#endif
#ifdef HAS_DHT11
    ,dht11_write
#endif
#ifdef HAS_MAX232_SOFTWARE
    ,halfduplex_write 
#endif
#ifdef HAS_BMP180
    ,bmp180_sinkWrite 
#endif
#ifdef HAS_DIGITAL_COUNTER
    ,flow_write
#endif
#ifdef HAS_ANALOG_INTEGRATOR
    ,anint_sinkWrite
#endif
};

*/

void sinks_init() {
#ifdef HAS_MAX232_SOFTWARE
    max232_init();
#endif

#ifdef HAS_I2C
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
