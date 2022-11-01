#include "net/net.h"
#include "leds.h"

void led_init() { 
#ifdef LED_PORTBIT
    LED_PORTBIT = 0;
    LED_TRISBIT = 0;
#endif
}

void led_off() {
#ifdef LED_PORTBIT
    LED_PORTBIT = 0;
#endif
}

void led_on() {
#ifdef LED_PORTBIT
    LED_PORTBIT = 1;
#endif
}

