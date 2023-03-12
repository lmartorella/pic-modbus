#ifndef __TICK_H
#define __TICK_H

#ifdef __cplusplus
extern "C" {
#endif

/**
 * Init tick timers
 */
void timers_init(void);

/**
 * To be called from ISR interrupt handler, optimized for minimal stack usage.
 * Update tick timers at ~Khz freq
 * Check flag of the timer for overflow, and in case increments the MSB tick bytes
 */
void timers_isr(void);

/**
 * Get current timer value in ticks
 */
TICK_TYPE timers_get(void);

#ifdef __cplusplus
}
#endif

#endif
