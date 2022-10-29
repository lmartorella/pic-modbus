#ifndef _APPIO_INCLUDE_
#define _APPIO_INCLUDE_

/**
 * General module for system-related functions
 */

/**
 * Init the IO module (display or notification led)
 */
void io_init();

/**
 * Clear and print string in the upper row of the display (status)
 */
void io_printlnStatus(const char* msg);

/**
 * Clear and print string in the below row (messages)
 */
void io_println(const char* msg);

#ifdef DEBUGMODE
/**
 * Print a char in the current row at the cursor (for debug builds)
 */
void io_printChDbg(char ch);
#endif

/**
 * Enumerates the reason of a reset. Mainly used for MCU codes
 */
typedef enum 
{
	/**
	 * This is reset by the server after boot
	 */
    RESET_NONE = 0,

	/**
	 * Power-on reset (e.g. after a power loss), POR
	 */
	RESET_POWER = 1,

	/**
	 * Brownout (low voltage reset), BOR
	 */
	RESET_BROWNOUT,

	/**
	 * Used by PIC18F, CM
	 */
	RESET_CONFIGMISMATCH,

	/**
	 * Watchdog reset, due to code loop, TO.
	 */
	RESET_WATCHDOG,

	/**
	 * Stack overflow/underflow (STKFUL, STKUNF).
	 */
	RESET_STACKFAIL,

	/**
	 * Reset line activated (push-button zero configuration or programmer connected)
	 */
	RESET_MCLR,

	/**
	 * Custom software exception. See g_lastException string
	 */
	RESET_EXC
} RESET_REASON;

/**
 * Contains the reset reason code
 */
extern RESET_REASON g_resetReason;

/**
 * Contains a pointer to the program memory, that contains the ASCIIZ exception string
 */
extern LAST_EXC_TYPE g_lastException;

/**
 * This should be called as very first line of code in MCUs to analyze the reset flags
 */
void sys_storeResetReason();

#ifdef _CONF_POSIX
/**
 * Log on log file (Posix only)
 */
void flog(const char* format, ...);
#else
// Not implemented in MCUs
#define flog(...) 
#endif

#endif
