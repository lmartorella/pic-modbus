#include "net/net.h"
#include "samples.h"
#include "samples/hardware/led.h"

#ifdef HAS_LED_BLINK

LedBlinkRegsiters blinker_regs;

static TICK_TYPE timer;
static _Bool state;

void blinker_init() {
    led_init();
    timer = timers_get();
    state = false;
    blinker_regs.period = TICKS_PER_SECOND;
}

_Bool blinker_conf() {
    // Always ok
    return true;
}

void blinker_poll() {
    TICK_TYPE elapsed = timers_get() - timer;
    if (elapsed > blinker_regs.period) {
        timer = timers_get();
        if ((state = !state)) {
            led_off();
        } else {
            led_on();
        }
    }
}

#endif
