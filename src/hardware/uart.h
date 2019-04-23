#ifndef UART_H
#define	UART_H

typedef struct {
    unsigned rc9 :1;
    unsigned ferr :1;
    unsigned oerr :1;
} UART_RX_MD;

#ifdef __XC8

// Use inline definition to avoid stack depths

// MAX485 levels
#define EN_TRANSMIT 1
#define EN_RECEIVE 0

#define uart_trasmit() RS485_PORT_EN = EN_TRANSMIT
#define uart_receive() RS485_PORT_EN = EN_RECEIVE

#define uart_init() \
    RS485_RCSTA.SPEN = 1; \
    RS485_RCSTA.RX9 = 1; \
    RS485_TXSTA.SYNC = 0; \
    RS485_TXSTA.TX9 = 1; \
    RS485_INIT_BAUD(); \
    /* Enable ports */ \
    RS485_TRIS_RX = 1; \
    RS485_TRIS_TX = 0; \
    /* Enable control ports */ \
    RS485_TRIS_EN = 0; \

#define uart_set_9b(b) RS485_TXSTA.TX9D = b
#define uart_write(b) RS485_TXREG = b
#define uart_read(data, md) \
    /* Check for errors BEFORE reading RCREG */ \
    (md)->oerr = RS485_RCSTA.OERR; \
    (md)->ferr = RS485_RCSTA.FERR; \
    /* read data to reset IF and FERR */ \
    (md)->rc9 = RS485_RCSTA.RX9D; \
    *(data) = RS485_RCREG;

#define uart_tx_fifo_empty() RS485_PIR_TXIF
#define uart_tx_fifo_empty_get_mask() RS485_PIE_TXIE
#define uart_tx_fifo_empty_set_mask(b) RS485_PIE_TXIE = b

#define uart_rx_fifo_empty() !RS485_PIR_RCIF
#define uart_rx_fifo_empty_get_mask() RS485_PIE_RCIE
#define uart_rx_fifo_empty_set_mask(b) RS485_PIE_RCIE = b

#define uart_enable_tx()  RS485_TXSTA.TXEN = 1
#define uart_disable_tx()  RS485_TXSTA.TXEN = 0
#define uart_enable_rx()   RS485_RCSTA.CREN = 1
#define uart_disable_rx()  RS485_RCSTA.CREN = 0

#else

void uart_init();
void uart_trasmit();
void uart_receive();
void uart_set_9b(BOOL b);
void uart_write(BYTE b);
void uart_read(BYTE* data, UART_RX_MD* md);

BOOL uart_tx_fifo_empty();
BOOL uart_tx_fifo_empty_get_mask();
void uart_tx_fifo_empty_set_mask(BOOL b);

BOOL uart_rx_fifo_empty();
BOOL uart_rx_fifo_empty_get_mask();
void uart_rx_fifo_empty_set_mask(BOOL b);

void uart_enable_tx();
void uart_disable_tx();
void uart_enable_rx();
void uart_disable_rx(); 

#endif
                                      
#endif	/* UART_H */

