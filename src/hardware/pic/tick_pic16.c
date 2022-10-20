#include "../../../../src/nodes/pch.h"
#include "../tick.h"

#if defined(_IS_PIC16F628_CARD) || defined(_IS_PIC16F887_CARD) || defined(_IS_PIC16F1827_CARD)

// Internal counter to store Ticks.  This variable is incremented in an ISR and
// therefore must be marked volatile to prevent the compiler optimizer from
// reordering code to use this value in the main context while interrupts are
// disabled.
static volatile BYTE s_ticksH = 0;

/*****************************************************************************
  Function:
	void timers_init(void)

  Summary:
	Initializes the Tick manager module.

  Description:
	Configures the Tick module and any necessary hardware resources.

  Precondition:
	None

  Parameters:
	None

  Returns:
  	None

  Remarks:
	This function is called only one during lifetime of the application.
  ***************************************************************************/
void timers_init(void)
{
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


/*****************************************************************************
  Function:
	WORD TickGet(void)

  Summary:
	Obtains the current Tick value.

  Description:
	This function retrieves the current Tick value, allowing timing and
	measurement code to be written in a non-blocking fashion.  This function
	retrieves the least significant 16 bits of the internal tick counter

  Precondition:
	None

  Parameters:
	None

  Returns:
  	Lower 16 bits of the current Tick value.
  ***************************************************************************/
WORD TickGet()
{
    CLRWDT();
    // 2-byte value to store Ticks.  
    WORD vTickReading;

	// Perform an Interrupt safe and synchronized read of the 48-bit
	// tick value
	do
	{
		TICK_INTCON_IE = 1;		// Enable interrupt
		NOP();                  // Manage TMR interrupts, if IF = 1
		TICK_INTCON_IE = 0;		// Disable interrupt
		((BYTE*)(&vTickReading))[0] = TICK_TMR;
        ((BYTE*)(&vTickReading))[1] = s_ticksH;
	} while (TICK_INTCON_IF);
	TICK_INTCON_IE = 1;			// Enable interrupt
	return vTickReading;
}

/*****************************************************************************
  Function:
	void timers_poll(void)

  Description:
	Updates the tick value when an interrupt occurs.

  Precondition:
	None

  Parameters:
	None

  Returns:
     The LSB of tick
  ***************************************************************************/
void timers_poll(void)
{
    if (TICK_INTCON_IF)
    {
        // Increment internal high tick counter
        s_ticksH++;
        // Reset interrupt flag
        TICK_INTCON_IF = 0;
    }
}

#endif
