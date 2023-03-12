#include <net/net.h>
#include "samples.h"

#define SYS_REGS_ADDRESS (0)

void samples_init() {
#ifdef HAS_LED_BLINK
    blinker_init();
#endif
}

void samples_poll() {
#ifdef HAS_LED_BLINK
    blinker_poll();
#endif
}

static uint16_t address;

static void be16toh(const uint16_t* dest, const uint16_t* src) {
    ((uint8_t*)dest)[1] = ((const uint8_t*)src)[0];
    ((uint8_t*)dest)[0] = ((const uint8_t*)src)[1];
}

_Bool regs_validateAddr() {
    uint8_t count = bus_cl_header.address.countL;
    be16toh(&address, &bus_cl_header.address.registerAddressBe);
    
    // Exposes the system registers in the rane 0-2
    if (address == SYS_REGS_ADDRESS) {
        if (count != SYS_REGS_COUNT) {
            bus_cl_exceptionCode = ERR_INVALID_SIZE;
            return false;
        }
        if (bus_cl_header.header.function != READ_HOLDING_REGISTERS) {
            bus_cl_exceptionCode = ERR_INVALID_FUNCTION;
            return false;
        }
        return true;
    }
    
#ifdef HAS_LED_BLINK
    if (address == LEDBLINK_REGS_ADDRESS) {
        if (count != LEDBLINK_REGS_COUNT) {
            bus_cl_exceptionCode = ERR_INVALID_SIZE;
            return false;
        }
        return true;
    }
#endif
    
    bus_cl_exceptionCode = ERR_INVALID_ADDRESS;
    return false;
}

_Bool regs_onReceive() {
#ifdef HAS_LED_BLINK
    if (address == LEDBLINK_REGS_ADDRESS) {
        memcpy(&blinker_regs, rs485_buffer, sizeof(LedBlinkRegsiters));
        return blinker_conf();
    }
#endif
    return false;
}

void regs_onSend() {
    if (address == SYS_REGS_ADDRESS) {
        memcpy(rs485_buffer, &regs_registers, sizeof(SYS_REGISTERS));
        return;
    }
    
#ifdef HAS_LED_BLINK
    if (address == LEDBLINK_REGS_ADDRESS) {
        memcpy(rs485_buffer, &blinker_regs, sizeof(LedBlinkRegsiters));
        return;
    }
#endif
}

