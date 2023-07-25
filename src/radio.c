#include "pic-modbus/radio.h"

#ifdef _CONF_RADIO

RS485_LINE_STATE rs485_state;
uint8_t rs485_buffer[RS485_BUF_SIZE];
_Bool rs485_isMarkCondition;

void rs485_init() {
}

uint8_t rs485_readAvail() {
}

_Bool rs485_writeInProgress() {
}

_Bool rs485_poll() {
}

void rs485_write(uint8_t size) {
}

void rs485_discard(uint8_t count) {
}

#endif
