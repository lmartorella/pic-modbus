#ifndef _MODBUS_UART_H
#define	_MODBUS_UART_H

#ifdef __cplusplus
extern "C" {
#endif

/**
 * Module that virtualize 8-bit UART support for bus wired communication from RS485 module.
 * Not using parity since PIC 16/18 doesn't have any hardware support, and having it in software
 * will probably kill performances.
 * Declared to decouple UART implementation, that can be native Microchip MCU or Linux based (via USB dongle).
 */
    
typedef struct {
    uint8_t data;
    UART_ERR_BITS errs;
} UART_LAST_CH;
extern UART_LAST_CH uart_lastCh;

void uart_init();
void uart_transmit();
void uart_receive();
void uart_write(uint8_t b);
void uart_read();

_Bool uart_tx_fifo_empty();
_Bool uart_rx_fifo_empty();

#ifdef __cplusplus
}
#endif

#endif	/* UART_H */

