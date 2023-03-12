#ifndef SAMPLE_CONFIG_H
#define	SAMPLE_CONFIG_H

/**
 * Enable/disable samples
 */
#define HAS_LED_BLINK

/**
 * Auto-select hardware based on activated samples
 */
#ifdef HAS_LED_BLINK
#define HAS_LED
#endif

#endif	/* SAMPLE_CONFIG_H */

