#include <string.h>
#include "net/bus_client.h"
#include "net/registers.h"
#include "net/rs485.h"

// At reset the values of the registers will remain, so the reset reason / error
// can be sent out
__persistent SYS_REGISTERS regs_registers;

/**
 * Module that implements the system sinks to reflect the node content.
 * Exposed as system ModBus registers
 */

void regs_onRead() {
    memcpy(rs485_buffer, &regs_registers, sizeof(SYS_REGISTERS));
}

void regs_onWrite() {
}

