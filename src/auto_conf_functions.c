#include <stdbool.h>
#include <stdint.h>
#include <string.h>
#include "net/appio.h"
#include "net/auto_conf_functions.h"
#include "net/bus_client.h"
#include "net/leds.h"
#include "net/persistence.h"

/**
 * Module that implements the system sinks to reflect the node content.
 * Exposed as system ModBus registers
 */

void autoconf_init() {
    bus_cl_stationAddress = pers_data.sec.address;
    // Address should be reset?
    if (g_resetReason == RESET_MCLR
#ifdef _IS_ETH_CARD
            // Bug of HW spec of PIC18?
            || g_resetReason == RESET_POWER
#endif
    ) {
        // Reset address
        bus_cl_stationAddress = pers_data.sec.address = UNASSIGNED_STATION_ADDRESS;
        pers_save();
    }

    if (bus_cl_stationAddress == UNASSIGNED_STATION_ADDRESS) {
        // Signal unattended secondary client, but doesn't auto-assign to avoid line clash at multiple boot
        led_on();
    }
}

static void storeAddress(uint8_t address) {
    pers_data.sec.address = bus_cl_stationAddress;
    pers_save();
    led_off();
}

/**
 * Register 0x0: get sinks count, sink status (reset reason), and exception code.
 */
void autoconf_readNodeStatus(AUTOCONF_NODE_STATUS* buffer) {
    buffer->functionCount = bus_cl_appFunctionCount;
    buffer->resetReason = g_resetReason;
    memset(buffer->errMsg, 0, sizeof(buffer->errMsg));
    if (g_resetReason == RESET_EXC) {
        strncpy(buffer->errMsg, (const char *)g_lastException, sizeof(buffer->errMsg));
    }
}

/**
 * Register 0x1: get sinks ID
 */
void autoconf_readSinkIds(FOURCC* ids) {
    for (uint8_t i = 0; i < bus_cl_appFunctionCount; i++) {
        ids[i] = bus_cl_appFunctions[i].id;
    }
}

/**
 * Register 0x2: get/set current node GUID as 16 raw bytes
 * Read: 16
 */
void autoconf_readNodeGuid(GUID* guid) {
    *guid = pers_data.deviceId;
}
void autoconf_writeNodeGuid(const GUID* guid) {
    pers_data.deviceId = *guid;
    // Have new GUID! Program it.
    pers_save();
}
