#include <net/net.h>
#include "./samples.h"

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
