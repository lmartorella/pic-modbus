#ifndef __TICK_H
#define __TICK_H

// All TICKS are stored as 32-bit unsigned integers.

// This value is used by TCP and other modules to implement timeout actions.
// For this definition, the Timer must be initialized to use a 1:256 prescalar
// in Tick.c.  

// Represents one second in Ticks
#define TICK_SECOND				(TICK_TYPE)(TICKS_PER_SECOND)
// Represents one minute in Ticks
#define TICK_MINUTE				(TICK_TYPE)(TICKS_PER_SECOND * 60ull)
// Represents one hour in Ticks
#define TICK_HOUR				(TICK_TYPE)(TICKS_PER_SECOND * 3600ull)

TICK_TYPE TickGet();
TICK_TYPE TickGetDiv256();
TICK_TYPE TickGetDiv64K();
void TickUpdate();

void timers_init();

#endif
