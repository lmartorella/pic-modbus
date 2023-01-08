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
 * Poll the interrupt flag of the timer for overflow, and in case increments the MSB tick bytes
 */
void timers_poll(void);

/**
 * Get current timer value in ticks
 */
TICK_TYPE timers_get(void);

#ifdef __cplusplus
}
#endif

#endif
