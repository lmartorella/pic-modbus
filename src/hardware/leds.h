#ifndef LEDS_H
#define	LEDS_H

/**
 * Functions to regulate notification led
 */

#ifdef HAS_LED

#define led_init() { LED_PORTBIT = 0; LED_TRISBIT = 0; }
#define led_off() { LED_PORTBIT = 0; }
#define led_on() { LED_PORTBIT = 1; }

#else

#define led_init() 
#define led_off() 
#define led_on() 

#endif

#endif	/* LEDS_H */

