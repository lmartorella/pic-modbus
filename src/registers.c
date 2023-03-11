#include <stdbool.h>
#include <stdint.h>
#include <string.h>
#include "net/bus_client.h"
#include "net/leds.h"
#include "net/persistence.h"
#include "net/registers.h"
#include "net/rs485.h"
#include "net/registers.h"

__persistent SYS_REGISTERS regs_registers;

/**
 * Module that implements the system sinks to reflect the node content.
 * Exposed as system ModBus registers
 */

void regs_init() {
    // Address should be reset?
    if (regs_registers.resetReason == RESET_MCLR
#ifdef _IS_ETH_CARD
            // Bug of HW spec of PIC18?
            || g_resetReason == RESET_POWER
#endif
    ) {
        // Reset address
        regs_registers.stationNode = pers_data.address = UNASSIGNED_STATION_ADDRESS;
        pers_save();
    }
    
    regs_registers.stationNode = pers_data.address;
    if (regs_registers.stationNode == UNASSIGNED_STATION_ADDRESS) {
        // Signal unattended secondary client, but doesn't auto-assign to avoid line clash at multiple boot
        led_on();
    }
}

void regs_onRead() {
    memcpy(rs485_buffer, &regs_registers, sizeof(SYS_REGISTERS));
}

void regs_onWrite() {
    memcpy(&regs_registers, rs485_buffer, sizeof(SYS_REGISTERS));
    pers_data.address = regs_registers.stationNode;
    pers_save();
    led_off();
}

