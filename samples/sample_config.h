#ifndef SAMPLE_CONFIG_H
#define	SAMPLE_CONFIG_H

/**
 * Enable/disable samples
 */
#define HAS_LED_BLINK
#define HAS_BMP180

/**
 * Auto-select hardware based on activated samples
 */
#ifdef HAS_LED_BLINK
#define HAS_LED
#endif
#ifdef HAS_BMP180
#define HAS_I2C
#endif

#endif	/* SAMPLE_CONFIG_H */

