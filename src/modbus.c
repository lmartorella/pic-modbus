#include "pic-modbus/modbus.h"

void modbus_init() {
    timers_init();
    rtu_cl_init();
}

_Bool modbus_poll() {
    return rtu_cl_poll();
}
