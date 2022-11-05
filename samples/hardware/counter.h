#ifndef DIG_COUNTER_H
#define	DIG_COUNTER_H

// Used for flow counter. Uses interrupts to not lose any tick.

void dcnt_interrupt();
void dcnt_init();
void dcnt_poll();

typedef struct {
    // Copied from persistence to better control atomicity of accesses. 
    // Ticks.
    uint32_t counter;
    // Should be enough for 200lt/min. Tick/secs.
    uint16_t flow;
} DCNT_DATA;
void dcnt_getDataCopy(DCNT_DATA* data);

#endif	/* COUNTER_H */

