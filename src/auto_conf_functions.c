#include <stdbool.h>
#include <stdint.h>
#include <string.h>
#include "net/appio.h"
#include "net/auto_conf_functions.h"
#include "net/bus_client.h"
#include "net/leds.h"
#include "net/persistence.h"
#include "net/rs485.h"

/**
 * Module that implements the system sinks to reflect the node content.
 * Exposed as system ModBus registers
 */

void autoconf_init() {
#if HARDCODED_STATION_ADDRESS
    bus_cl_stationAddress = HARDCODED_STATION_ADDRESS;
#else    
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
    
    bus_cl_stationAddress = pers_data.sec.address;
    if (bus_cl_stationAddress == UNASSIGNED_STATION_ADDRESS) {
        // Signal unattended secondary client, but doesn't auto-assign to avoid line clash at multiple boot
        led_on();
    }
#endif
}

static void storeAddress(uint8_t address) {
    pers_data.sec.address = bus_cl_stationAddress;
    pers_save();
    led_off();
}

/**
 * Register 0x0: get sinks count, sink status (reset reason), and exception code.
 */
void autoconf_readNodeStatus() {
#define MSG_1 ((AUTOCONF_NODE_STATUS*)rs485_buffer)
    MSG_1->functionCount = bus_cl_appFunctionCount;
    MSG_1->resetReason = g_resetReason;
    memcpy(MSG_1->errMsg, (const void*)g_lastException, sizeof(MSG_1->errMsg));
/*    for (uint8_t i = 0; i < sizeof(MSG_1->errMsg); i++) {
        MSG_1->errMsg[i] = ((uint8_t*)g_lastException)[i];
    } */
}

/**
 * Register 0x1: get sinks ID
 */
void autoconf_readSinkIds() {
#define MSG_2 ((FOURCC*)rs485_buffer)
    for (uint8_t i = 0; i < bus_cl_appFunctionCount; i++) {
        MSG_2[i] = bus_cl_appFunctionIds[i];
    }
}

/**
 * Register 0x2: get/set current node GUID as 16 raw bytes
 * Read: 16
 */
void autoconf_readNodeGuid() {
#define MSG_3 ((GUID*)rs485_buffer)
    *MSG_3 = pers_data.deviceId;
}
void autoconf_writeNodeGuid() {
#define MSG_3 ((GUID*)rs485_buffer)
    pers_data.deviceId = *MSG_3;
    // Have new GUID! Program it.
    pers_save();
}
