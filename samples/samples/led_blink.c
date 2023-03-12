#include "net/net.h"
#include "../ssamples"
#include "./hardware/led.h"

#ifdef HAS_LED_BLINK

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
