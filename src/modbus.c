#include "pic-modbus/modbus.h"

void modbus_init() {
    timers_init();
    rtu_cl_init();
    rs485_init();
}

_Bool modbus_poll() {
    _Bool active = rs485_poll();
    if (rtu_cl_poll()) {
        active = true;
    }
    return active;
}
