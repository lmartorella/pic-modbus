#ifndef __TICK_H
#define __TICK_H

/**
 * Imported from Microchip Ethernet library.
 * Module that offers tick-based poll timers.
 */

// All TICKS are stored as 32-bit unsigned integers.

// This value is used by TCP and other modules to implement timeout actions.
// For this definition, the Timer must be initialized to use a 1:256 prescalar
// in Tick.c.  

// Represents one second in Ticks
#define TICK_SECOND				(TICK_TYPE)(TICKS_PER_SECOND)

TICK_TYPE timers_get();

/**
 * For ETH code
 */
#define TickGet timers_get

/**
 * Used by ETH code
 */
TICK_TYPE TickGetDiv256();

/**
 * Used by ETH code
 */
TICK_TYPE TickGetDiv64K();

/**
 * Poll the interrupt flag of the timer for overflow, and in case increments the MSB tick bytes
 */
void timers_poll();

/**
 * Init tick timers
 */
void timers_init();

#endif
