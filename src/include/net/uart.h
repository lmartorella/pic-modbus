#ifndef UART_H
#define	UART_H

/**
 * Module that virtualize 8-bit UART support for bus wired communication from RS485 module.
 * Not using parity since PIC 16/18 doesn't have any hardware support, and having it in software
 * will probably kill performances.
 * Declared to decouple UART implementation, that can be native Microchip MCU or Linux based (via USB dongle).
 */

typedef struct {
    // Frame error occurred?
    unsigned frameErr :1;
    // Overflow error occurred?
    unsigned overrunErr :1;
} UART_RX_MD;

void uart_init();
void uart_transmit();
void uart_receive();
void uart_write(uint8_t b);
void uart_read(uint8_t* data, UART_RX_MD* md);

_Bool uart_tx_fifo_empty();
_Bool uart_rx_fifo_empty();

void uart_enable_tx();
void uart_disable_tx();
void uart_enable_rx();
void uart_disable_rx(); 
                                      
#endif	/* UART_H */

