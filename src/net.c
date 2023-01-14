#include "net/net.h"

void net_prim_init(uint16_t serverUdpPort) {
    timers_init();
    io_init();
    led_init();
    pers_load();
    bus_srv_init(serverUdpPort);
    prot_init();
    rs485_init();
}

static void net_cl_init_address() {
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

void net_cl_init() {
    timers_init();
    io_init();
    led_init();
    pers_load();

    bus_cl_init();
    net_cl_init_address();

    prot_init();
    rs485_init();
}

#define NET_POLL_IMPL(BUS_POLL, PROT_POLL) \
    CLRWDT(); \
    _Bool active = BUS_POLL(); \
    active = PROT_POLL() || active; \
    active = rs485_poll() || active; \
    return pers_poll() || active; \

_Bool net_prim_poll() {
    NET_POLL_IMPL(bus_srv_poll, prot_srv_poll);
}
_Bool net_cl_poll() {
    NET_POLL_IMPL(bus_cl_poll, prot_cl_poll);
}
