#include "net/net.h"

void net_init() {
    timers_init();
    led_init();
    pers_load();
    bus_cl_init();
    regs_init();
    rs485_init();
}

_Bool net_poll() {
    _Bool active = rs485_poll();
    if (bus_cl_poll()) {
        active = true;
    }
    if (pers_poll()) {
        active = true;
    }
    return active;
}
