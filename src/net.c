#include "net/net.h"

#define NET_INIT_IMPL(BUS_INIT) \
    timers_init(); \
    io_init(); \
    led_init(); \
    pers_load(); \
    BUS_INIT; \
    prot_init(); \
    rs485_init(); \

void net_prim_init(uint16_t serverUdpPort) {
    NET_INIT_IMPL(bus_srv_init(serverUdpPort));
}
void net_cl_init() {
    NET_INIT_IMPL(bus_cl_init());
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
