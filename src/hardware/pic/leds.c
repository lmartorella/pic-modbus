#include "../../pch.h"
#include "../../leds.h"

void led_init() { 
    LED_PORTBIT = 0;
    LED_TRISBIT = 0;
}

void led_off() {
    LED_PORTBIT = 0;
}

void led_on() {
    LED_PORTBIT = 1;
}

