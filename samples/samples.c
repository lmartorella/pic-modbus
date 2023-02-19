#include <net/net.h>
#include "./samples.h"

// REGISTER SINKS
// Static allocation of sinks
const uint8_t bus_cl_sysFunctionCount = 3;
const ReadHandler bus_cl_sysFunctionReadHandlers[3] = {
    (ReadHandler)autoconf_readNodeStatus,
    (ReadHandler)autoconf_readSinkIds,
    (ReadHandler)autoconf_readNodeGuid
};
const WriteHandler bus_cl_sysFunctionWriteHandlers[3] = {
    NULL,
    NULL,
    (WriteHandler)autoconf_writeNodeGuid
};

const uint8_t bus_cl_appFunctionCount = 0;
const ReadHandler bus_cl_appFunctionReadHandlers[0] = { };
const WriteHandler bus_cl_appFunctionWriteHandlers[0] = { };
const uint8_t bus_cl_appFunctionReadHandlerSizes[0] = { };
const uint8_t bus_cl_appFunctionWriteHandlerSizes[0] = { };
const FOURCC bus_cl_appFunctionIds[0] = { };

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

static TICK_TYPE test_timer;
static _Bool test_led;

static void test_timer_init() {
    test_timer = timers_get();
    test_led = false;
}

static void test_timer_poll() {
    TICK_TYPE elapsed = timers_get() - test_timer;
    if (elapsed > TICKS_PER_SECOND) {
        test_timer = timers_get();
        test_led = !test_led;
        if (test_led) {
            led_off();
        } else {
            led_on();
        }
    }
}

void sinks_init() {
    test_timer_init();
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
    test_timer_poll();
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
