#ifndef RS485_H
#define	RS485_H

/**
 * High-level RS485 operation module virtualization. It basically encapsulate a 8-bit UART
 * with additional direction bit. 
 */

/**
 * State of the RS485 line
 */
typedef enum {
    // Receiving, all OK
    RS485_LINE_RX,
    // End of transmitting, in disengage line period
    RS485_LINE_TX_DISENGAGE,
    // Transmitting, data
    RS485_LINE_TX,
    // After TX engaged, wait before transmitting
    RS485_LINE_WAIT_FOR_START_TRANSMIT
} RS485_STATE;

extern RS485_STATE rs485_state;

/**
 * Initialize asynchronous mode, but only half-duplex is used
 */
void rs485_init(void);

/**
 * To feed/receive channel
 */
void rs485_interrupt(void);

/**
 * Poll as much as possible (using internal timer)
 * Returns true if active and require polling
 */
_Bool rs485_poll(void);

// Enqueue bytes to send. Buffer is copied (max. 32 bytes)
void rs485_write(const uint8_t* data, uint8_t size);
// Read data, if available.
__bit rs485_read(uint8_t* data, uint8_t size);

/**
 * Get count of available bytes in the read buffer
 */
uint8_t rs485_readAvail();

/**
 * Get count of available bytes in the write buffer
 */
uint8_t rs485_writeAvail(void);

#endif	/* RS485_H */

