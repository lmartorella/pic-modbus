#include "../../../../src/nodes/pch.h"
#include "../../timers.h"

// Internal counter to store Ticks.  This variable is incremented in an ISR and
// therefore must be marked volatile to prevent the compiler optimizer from
// reordering code to use this value in the main context while interrupts are
// disabled.
static volatile uint32_t dwInternalTicks = 0;

// 6-byte value to store Ticks.  Allows for use over longer periods of time.
static uint8_t vTickReading[6];

static void GetTickCopy(void);


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
    TICK_TMRL = 0;
    TICK_TMRH = 0;
    
	// Set up the timer interrupt
    TICK_INTCON_IF = 0;

    // Set up prescaler and other stuff
    TICK_TCON = TICK_TCON_DATA;   

    TICK_INTCON_IE = 1;		// Enable interrupt
}

/*****************************************************************************
  Function:
	static void GetTickCopy(void)

  Summary:
	Reads the tick value.

  Description:
	This function performs an interrupt-safe and synchronized read of the
	48-bit Tick value.

  Precondition:
	None

  Parameters:
	None

  Returns:
  	None
  ***************************************************************************/
static void GetTickCopy(void)
{
	// Perform an Interrupt safe and synchronized read of the 48-bit
	// tick value
	do
	{
		TICK_INTCON_IE = 1;		// Enable interrupt
		NOP();
		TICK_INTCON_IE = 0;		// Disable interrupt
		vTickReading[0] = TICK_TMRL;
		vTickReading[1] = TICK_TMRH;
		*((uint32_t*)&vTickReading[2]) = dwInternalTicks;
	} while(TICK_INTCON_IF);
	TICK_INTCON_IE = 1;			// Enable interrupt
}


/*****************************************************************************
  Function:
	uint32_t TickGet(void)

  Summary:
	Obtains the current Tick value.

  Description:
	This function retrieves the current Tick value, allowing timing and
	measurement code to be written in a non-blocking fashion.  This function
	retrieves the least significant 32 bits of the internal tick counter,
	and is useful for measuring time increments ranging from a few
	microseconds to a few hours.  Use TickGetDiv256 or TickGetDiv64K for
	longer periods of time.

  Precondition:
	None

  Parameters:
	None

  Returns:
  	Lower 32 bits of the current Tick value.
  ***************************************************************************/
uint32_t TickGet(void)
{
	GetTickCopy();
	return *((uint32_t*)&vTickReading[0]);
}

/*****************************************************************************
  Function:
	uint32_t TickGetDiv256(void)

  Summary:
	Obtains the current Tick value divided by 256.

  Description:
	This function retrieves the current Tick value, allowing timing and
	measurement code to be written in a non-blocking fashion.  This function
	retrieves the middle 32 bits of the internal tick counter,
	and is useful for measuring time increments ranging from a few
	minutes to a few weeks.  Use TickGet for shorter periods or TickGetDiv64K
	for longer ones.

  Precondition:
	None

  Parameters:
	None

  Returns:
  	Middle 32 bits of the current Tick value.
  ***************************************************************************/
uint32_t TickGetDiv256(void)
{
	uint32_t dw;

	GetTickCopy();
	((uint8_t*)&dw)[0] = vTickReading[1];	// Note: This copy must be done one
	((uint8_t*)&dw)[1] = vTickReading[2];	// byte at a time to prevent misaligned
	((uint8_t*)&dw)[2] = vTickReading[3];	// memory reads, which will reset the PIC.
	((uint8_t*)&dw)[3] = vTickReading[4];

	return dw;
}

/*****************************************************************************
  Function:
	uint32_t TickGetDiv64K(void)

  Summary:
	Obtains the current Tick value divided by 64K.

  Description:
	This function retrieves the current Tick value, allowing timing and
	measurement code to be written in a non-blocking fashion.  This function
	retrieves the most significant 32 bits of the internal tick counter,
	and is useful for measuring time increments ranging from a few
	days to a few years, or for absolute time measurements.  Use TickGet or
	TickGetDiv256 for shorter periods of time.

  Precondition:
	None

  Parameters:
	None

  Returns:
  	Upper 32 bits of the current Tick value.
  ***************************************************************************/
uint32_t TickGetDiv64K(void)
{
	uint32_t dw;

	GetTickCopy();
	((uint8_t*)&dw)[0] = vTickReading[2];	// Note: This copy must be done one
	((uint8_t*)&dw)[1] = vTickReading[3];	// byte at a time to prevent misaligned
	((uint8_t*)&dw)[2] = vTickReading[4];	// memory reads, which will reset the PIC.
	((uint8_t*)&dw)[3] = vTickReading[5];

	return dw;
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
        dwInternalTicks++;
        
        // Reset interrupt flag
        TICK_INTCON_IF = 0;
    }
}
