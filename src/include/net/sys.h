#ifndef SYS_H
#define	SYS_H

#ifdef	__cplusplus
extern "C" {
#endif

/**
 * Enumerates the reason of a reset. Contains MCU codes and code exceptions
 */
typedef enum {
	/**
	 * This can happen if the reset is invoked by the code (e.g remotely)
	 */
    RESET_NONE = 0,

    // 0x1-0xf: Hardware reset reasons

	/**
	 * Power-on reset (e.g. after a power loss), POR
	 */
	RESET_POWER = 1,

	/**
	 * Brownout (low voltage reset), BOR
	 */
	RESET_BROWNOUT = 2,

	/**
	 * Used by PIC18F, CM
	 */
	RESET_CONFIGMISMATCH = 3,

	/**
	 * Watchdog reset, due to code loop, TO.
	 */
	RESET_WATCHDOG = 4,

	/**
	 * Stack overflow/underflow (STKFUL, STKUNF).
	 */
	RESET_STACKFAIL = 5,

	/**
	 * Reset line activated (push-button zero configuration or programmer connected)
	 */
	RESET_MCLR = 6,

    // RS485 errors
            
    /**
     * Data underrun in read from the serial line. OERR from hardware
     */
    EXC_CODE_RS485_READ_UNDERRUN = 0x10,

    /**
     * Data overrun in read from the serial line, buffer not dequeued.
     */
    EXC_CODE_RS485_READ_OVERRUN = 0x11,

    /**
     * Mismatch between read data and processed data (discard)
     */
    EXC_CODE_RS485_DISCARD_MISMATCH = 0x12,
            
    // Generic applicative fatal errors

    /**
     * Function/device not polled enough
     */
    ERR_DEVICE_DEADLINE_MISSED = 0x20,
            
    /**
     * Function/device generic HW unrecoverable fail
     */
    ERR_DEVICE_HW_FAIL = 0x21,

    /**
     * Function/device read overrun
     */
    ERR_DEVICE_READ_OVERRUN = 0x22,

    /**
     * Function/device HW communication error, missing ack, etc..
     */
    ERR_DEVICE_HW_NOT_ACK = 0x23

} SYS_RESET_REASON;
extern __persistent SYS_RESET_REASON sys_resetReason;

/**
 * This should be called as very first line of code in MCUs to analyze the reset flags
 */
void sys_init();

// Reset the device with sys (non-hw) error
#define sys_fatal(code) {\
    sys_resetReason = code;\
    RESET();\
}

#ifdef	__cplusplus
}
#endif

#endif	/* SYS_H */

