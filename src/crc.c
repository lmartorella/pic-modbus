#include "configuration.h"
#include "pic-modbus/crc.h"

#ifdef _CONF_RS485

// Only RS485 has CRC

uint16_t crc16;

void crc_reset() {
    crc16 = 0xffff;
}

void crc_update(uint8_t ch) {
    crc16 ^= ch;
    for (uint8_t i = 8; i != 0; i--) {
        if (crc16 & 1) {
            crc16 >>= 1;
            crc16 ^= 0xA001;
        } else {
            crc16 >>= 1;
        }
    }
}

#endif
