#ifndef DIG_COUNTER_H
#define	DIG_COUNTER_H

// Used for flow counter. Uses interrupts to not lose any tick.
#ifdef HAS_DIGITAL_COUNTER

void dcnt_interrupt();
void dcnt_init();
void dcnt_poll();

typedef struct {
    // Copied from persistence to better control atomicity of accesses. 
    // Ticks.
    DWORD counter;
    // Should be enough for 200lt/min. Tick/secs.
    WORD flow;
} DCNT_DATA;
void dcnt_getDataCopy(DCNT_DATA* data);

#endif

#endif	/* COUNTER_H */

