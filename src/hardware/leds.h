#ifndef LEDS_H
#define	LEDS_H

#ifdef HAS_LED

#define led_init() { LED_PORTBIT = 0; LED_TRISBIT = 0; }
#define led_off() { LED_PORTBIT = 0; }
#define led_on() { LED_PORTBIT = 1; }
#define led_isOn() (LED_PORTBIT)

#else

#define led_init() 
#define led_off() 
#define led_on() 
#define led_isOn() (0)

#endif

#endif	/* LEDS_H */

