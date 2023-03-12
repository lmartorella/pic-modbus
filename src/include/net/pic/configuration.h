#ifndef _STD_CONF_H
#define _STD_CONF_H

#include <stdbool.h>
#include <stdint.h>
#include <xc.h>

#define RS485_BAUD 19200
#define STATION_NODE (1)

// XC8 compiler is little-endian
#define le16toh(b) (b)
#define htole16(b) (b)

/**
 * Loads the MCU headers and additional fuses based on board configuration
 */
#if defined(_CONF_MICRO_BEAN)
#include "fuses_micro_bean.h"
#define _IS_PIC16F1827_CARD

#else
#error Missing configuration
#endif

#define _XTAL_FREQ SYSTEM_CLOCK

// Reset the device with sys (non-hw) error
#define fatal(code) {\
    regs_registers.resetReason = code;\
    RESET();\
}

#endif
