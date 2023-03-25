#include "net/net.h"

// Internal counter to store Ticks.  This variable is incremented in an ISR and
// therefore must be marked volatile to prevent the compiler optimizer from
// reordering code to use this value in the main context while interrupts are
// disabled.
static volatile uint8_t s_ticksH = 0;

void timers_init() {
	// Use Timer0 (that prescales to 1:256)
    // Initialize the time
    TICK_TMR = 0;
    
	// Set up the timer interrupt
    TICK_INTCON_IF = 0;

    // Set up prescaler and other stuff
    TICK_TCON |= TICK_TCON_1DATA;   
    TICK_TCON &= ~TICK_TCON_0DATA;   

    TICK_INTCON_IE = 1;		// Enable interrupt
}


uint16_t timers_get() {
    CLRWDT();
    // 2-byte value to store Ticks.  
    uint16_t vTickReading;

	// Perform an Interrupt safe and synchronized read of the 48-bit
	// tick value
	do {
		TICK_INTCON_IE = 1;		// Enable interrupt
		NOP();                  // Manage TMR interrupts, if IF = 1
		TICK_INTCON_IE = 0;		// Disable interrupt
		((uint8_t*)(&vTickReading))[0] = TICK_TMR;
        ((uint8_t*)(&vTickReading))[1] = s_ticksH;
	} while (TICK_INTCON_IF);
	TICK_INTCON_IE = 1;			// Enable interrupt
	return vTickReading;
}

void timers_isr() {
    if (TICK_INTCON_IF) {
        // Increment internal high tick counter
        s_ticksH++;
        // Reset interrupt flag
        TICK_INTCON_IF = 0;
    }
}
