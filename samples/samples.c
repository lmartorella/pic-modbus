#include <net/net.h>
#include "./samples.h"

static void noop() { }

// REGISTER SINKS
// Static allocation of sinks
const uint8_t bus_cl_functionCount = 6;
const ReadHandler bus_cl_functionReadHandlers[6] = {
    (ReadHandler)autoconf_readNodeStatus,
    (ReadHandler)autoconf_readSinkIds,
    (ReadHandler)autoconf_readNodeGuid,
    
    (ReadHandler)bmp180_read_reg1,
    (ReadHandler)bmp180_read_reg2,
    (ReadHandler)bmp180_read_reg3,
};
const WriteHandler bus_cl_functionWriteHandlers[6] = {
    (ReadHandler)autoconf_writeNodeStatus,
    noop,
    (WriteHandler)autoconf_writeNodeGuid,
    
    (ReadHandler)bmp180_write_reg1,
    noop,
    noop,
};

const uint8_t autoconf_appFunctionCount = 1;
const FOURCC autoconf_appFunctionIds[1] = { SINK_BMP180_ID };

#ifdef HAS_SAMPLE_LED_BLINK

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

#endif

void sinks_init() {
#ifdef HAS_SAMPLE_LED_BLINK
    test_timer_init();
#endif
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
#ifdef HAS_BMP180
    bmp180_poll();
#endif
#ifdef HAS_SAMPLE_LED_BLINK
    test_timer_poll();
#endif
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
