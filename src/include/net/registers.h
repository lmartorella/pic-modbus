#ifndef _SYS_INCLUDE_
#define _SYS_INCLUDE_

#include "configuration.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * General module for system-related functions
 */

/**
 * This should be called as very first line of code in MCUs to analyze the reset flags
 */
void regs_init();

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

} RESET_REASON;

/**
 * Holding rgisters [0-2]
 */
typedef struct {
    /**
     * See RESET_REASON
     */
    RESET_REASON resetReason;
    uint8_t _filler2;
    
    /**
     * Count of CRC errors in the reading period
     */
    uint16_t crcErrors;
} SYS_REGISTERS;

#define SYS_REGS_ADDRESS (0)
#define SYS_REGS_COUNT (sizeof(SYS_REGISTERS) / 2)

// In RAM
extern __persistent SYS_REGISTERS regs_registers;

/**
 * Validate request of read/write a register range. Header to check: bus_cl_header
 */
_Bool regs_validateAddr();

/**
 * Called when the registers (sys or app) are about to be read (sent out).
 * The rs485_buffer contains the data.
 */
_Bool regs_onReceive();

/**
 * Called when the registers (sys or app) was written
 * The rs485_buffer should be filled with the data.
 */
void regs_onSend();

#ifdef __cplusplus
}
#endif

#endif
