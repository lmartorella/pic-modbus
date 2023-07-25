#ifndef RADIO_H
#define	RADIO_H

#include <stdint.h>
#include "configuration.h"

#ifdef	__cplusplus
extern "C" {
#endif

void rs485_init();
_Bool rs485_poll();
extern _Bool rs485_isMarkCondition;
extern uint8_t rs485_buffer[RS485_BUF_SIZE];
void rs485_write(uint8_t size);
void rs485_discard(uint8_t count);
uint8_t rs485_readAvail();
_Bool rs485_writeInProgress();

typedef enum {
    // Receiving, all OK
    RS485_LINE_RX,
    // End of transmitting, in disengage line period
    RS485_LINE_TX_DISENGAGE,
    // Transmitting, data
    RS485_LINE_TX,
    // After TX engaged, wait before transmitting
    RS485_LINE_WAIT_FOR_START_TRANSMIT
} RS485_LINE_STATE;
extern RS485_LINE_STATE rs485_state;

#ifdef	__cplusplus
}
#endif

#endif	/* RADIO_H */

