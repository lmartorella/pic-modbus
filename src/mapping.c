#include <string.h>
#include "net/bus_client.h"
#include "net/mapping.h"
#include "net/rs485.h"

// At reset the values of the registers will remain, so the reset reason / error
// can be sent out
__persistent SYS_REGISTERS regs_registers;

/**
 * Module that implements the system sinks to reflect the node content.
 * Exposed as system ModBus registers
 */

_Bool regs_validateAddr() {
    // Only supports whole read
    if (bus_cl_header.address.registerAddressH != 0 || bus_cl_header.address.registerAddressL != 0) {
        // Invalid address, return error
        bus_cl_exceptionCode = ERR_INVALID_ADDRESS;
        return false;
    }
    if (bus_cl_header.address.countH != 0 || bus_cl_header.address.countL != SYS_REGS_COUNT) {
        // Invalid size, return error
        bus_cl_exceptionCode = ERR_INVALID_SIZE;
        return false;
    }
    // Only supports read
    if (bus_cl_header.header.function != READ_HOLDING_REGISTERS) {
        // Invalid size, return error
        bus_cl_exceptionCode = ERR_INVALID_FUNCTION;
        return false;
    }
    return true;
}

_Bool regs_onReceive() {
    // Never called
    bus_cl_exceptionCode = ERR_INVALID_FUNCTION;
    return false;
}

void regs_onSend() {
    memcpy(rs485_buffer, &regs_registers, sizeof(SYS_REGISTERS));
}

