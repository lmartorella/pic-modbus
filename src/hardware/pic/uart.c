#include "net/net.h"

// MAX485 line drive
#define EN_TRANSMIT 1
#define EN_RECEIVE 0

// MPLAB Sim has issue in simulating UART stimuli when the TXEN is toggled at runtime

#ifdef MPLAB_SIM
#define uart_init_rx_tx() RS485_TXSTA.TXEN = 1; RS485_RCSTA.CREN = 1;
#define uart_enable_tx()
#define uart_disable_tx()
#define uart_enable_rx()
#define uart_disable_rx()
#else
#define uart_init_rx_tx() RS485_TXSTA.TXEN = 0; RS485_RCSTA.CREN = 0;
#define uart_enable_tx() RS485_TXSTA.TXEN = 1;
#define uart_disable_tx() RS485_TXSTA.TXEN = 0;
#define uart_enable_rx() RS485_RCSTA.CREN = 1;
#define uart_disable_rx() RS485_RCSTA.CREN = 0;
#endif

void uart_init() {
    RS485_RCSTA.SPEN = 1;
    RS485_TXSTA.SYNC = 0;
    RS485_INIT_BAUD();
    /* Enable ports */
    RS485_TRIS_RX = 1;
    RS485_TRIS_TX = 0;
    /* Enable control ports */
    RS485_TRIS_EN = 0;
    /* Enable RX/TX periphericals */
    uart_init_rx_tx();

    uart_receive();
}

void uart_transmit() {
    // Truncate reading
    uart_disable_rx();
    // Enable UART transmit.
    uart_enable_tx();
    // Set RS485 transmit mode
    RS485_PORT_EN = EN_TRANSMIT;
}

void uart_receive() {
    uart_disable_tx();
    uart_enable_rx();
    
    // Set RS485 receive mode
    RS485_PORT_EN = EN_RECEIVE;
}

void uart_write(uint8_t b) {
    RS485_TXREG = b;
}

void uart_read(uint8_t* data, UART_RX_MD* md) {
    /* Check for errors BEFORE reading RCREG */
    (md)->overrunErr = RS485_RCSTA.OERR;
    (md)->frameErr = RS485_RCSTA.FERR;
    *(data) = RS485_RCREG;
}

_Bool uart_tx_fifo_empty() {
    return RS485_PIR_TXIF;
}

_Bool uart_rx_fifo_empty() {
    return !RS485_PIR_RCIF;
}

