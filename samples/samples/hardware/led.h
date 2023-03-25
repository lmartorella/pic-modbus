#ifndef LEDS_H
#define	LEDS_H

/**
 * Functions to regulate the notification led in the micro-bean board
 */

#ifdef __cplusplus
extern "C" {
#endif

void led_init(void);
void led_off(void);
void led_on(void);

#ifdef __cplusplus
}
#endif

#endif	/* LEDS_H */

