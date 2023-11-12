#include "pic-modbus/modbus.h"

void modbus_init() {
    timers_init();
    bus_cl_init();
    rs485_init();
}

_Bool modbus_poll() {
    _Bool active = rs485_poll();
    if (bus_cl_poll()) {
        active = true;
    }
    return active;
}
