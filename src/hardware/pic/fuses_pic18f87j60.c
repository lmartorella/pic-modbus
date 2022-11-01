#include "net.h"
#include "appio.h"
    
// ==== ETH CARD based on 18f87j60

// For the list of fuses, run "mcc18.exe --help-config -p=18f87j60"

// Enable WDT, 1:256 prescaler
#pragma config WDT = ON
#pragma config WDTPS = 256

// Enable stack overflow reset en bit
#pragma config STVR = ON

// Microcontroller mode ECCPA/P2A multiplexed (not used!)
//#pragma config ECCPMX = 1/0
//#pragma config CCP2MX = 1/0

// Ethernet led enabled. RA0/1 are multiplexed with LEDA/LEDB
#pragma config ETHLED = ON

// Fail-safe clock monitor enabled (switch to internal oscillator in case of osc failure)
#pragma config FCMEN = OFF
// Two-speed startup disabled (Internal/External Oscillator Switchover)
#pragma config IESO = OFF

// Disable extended instruction set (not supported by XC compiler)
#pragma config XINST = OFF

// FOSC = HS, no pll, INTRC disabled
#pragma config FOSC = HS, FOSC2 = ON

// Debugger
#ifdef __DEBUG
#pragma config DEBUG = ON
#else
#pragma config DEBUG = OFF
#endif
    
// No code protection
#pragma config CP0 = OFF


void sys_enableInterrupts() {
    // Disable low/high interrupt mode
    RCONbits.IPEN = 0;		
    INTCONbits.GIE = 1;
    INTCONbits.PEIE = 1;
    INTCON2bits.TMR0IP = 0;		// TMR0 Low priority
    
    // Enable low priority interrupt on transmit
    RS485_INIT_INT();
}
    
// The pointer is pointing to ROM space that will not be reset
// otherwise after the RESET the variable content can be lost.
static __persistent LAST_EXC_TYPE g_exception;

// Check RCON and STKPTR register for anormal reset cause
void sys_storeResetReason()
{
    // Disable all A/D channels
    ADCON1 |= 0xF;
    
    if (!RCONbits.NOT_RI)
    {
	// Software exception. 
	// Obtain last reason from appio.h 
	g_resetReason = RESET_EXC;
	g_lastException = g_exception;
	RCONbits.NOT_RI = 1;
    }
    else if (!RCONbits.NOT_POR)
    {
	// Normal Power-on startup. Ok.
	g_resetReason = RESET_POWER;
	RCONbits.NOT_POR = 1;
	RCONbits.NOT_BOR = 1;
    }    
    else if (!RCONbits.NOT_BOR)
    {
	// Brown-out reset. Low voltage.
	g_resetReason = RESET_BROWNOUT;
	RCONbits.NOT_POR = 1;
	RCONbits.NOT_BOR = 1;
    }    
/*
    else if (!RCONbits.NOT_CM)
    {
	    // Configuration mismatch reset. EEPROM fail.
	    _reason = RESET_CONFIGMISMATCH;
    }
*/
    else if (!RCONbits.NOT_TO)
    {
	// Watchdog reset. Loop detected.
	g_resetReason = RESET_WATCHDOG;
	RCONbits.NOT_TO = 1;
    }
    else if (STKPTRbits.STKFUL || STKPTRbits.STKUNF)
    {
	// Stack underrun/overrun reset. 
	g_resetReason = RESET_STACKFAIL;
	STKPTRbits.STKFUL = 0;
	STKPTRbits.STKUNF = 0;
    }
    else
    {
	// Else it was reset manually (MCLR)
	g_resetReason = RESET_MCLR;
    }

    g_exception = 0;
    CLRWDT();
}

// Long (callable) version of fatal
void fatal(const char* str)
{
    g_exception = (LAST_EXC_TYPE)str;
    __delay_ms(30);
    RESET(); // generates RCON.RI
}
