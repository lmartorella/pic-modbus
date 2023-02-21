#include "net/net.h"

/**
 * Was the node be acknowledged by the server? (auto-configuration)
 */
static __bit s_acknowledged;

void net_prim_init(uint16_t serverUdpPort) {
    // Prepare address
    s_acknowledged = false;

    timers_init();
    led_init();
    pers_load();
    bus_srv_init(serverUdpPort);
    autoconf_init();
    rs485_init();
}

void net_cl_init() {
    timers_init();
    led_init();
    pers_load();
    bus_cl_init();
    autoconf_init();
    rs485_init();
}

_Bool net_prim_poll() {
    _Bool active = bus_srv_poll(); 
    active = rs485_poll() || active;
    return pers_poll() || active;
}

_Bool net_cl_poll() {
    _Bool active = rs485_poll();
    if (bus_cl_poll()) {
        active = true;
    }
    if (pers_poll()) {
        active = true;
    }
    return active;
}
