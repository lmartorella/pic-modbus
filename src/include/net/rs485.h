#ifndef RS485_H
#define	RS485_H

#include "configuration.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * High-level generic RS485 operation module virtualization. 
 * It basically encapsulate a 8-bit UART with additional direction bit (usually RTS on RS485 USB dongles).
 */

/**
 * Initialize asynchronous mode, but only half-duplex is used
 */
void rs485_init(void);

/**
 * When the bus is engaged and a packet is transiting, poll should be called with 
 * a frequency at least 0.5 character period (e.g. 280us on 19200 baud).
 * When the bus is idle instead, the poll can be less intensive, depending 
 * on the HW buffer size of the UART in use (e.g. 2 characters, so 1ms on 19200 baud, on a PIC MCU, or even 
 * slower on a more buffered UART, but at least able to detect a message before his end).
 * Returns `true` if the bus is active (so fast poll is required).
 */
_Bool rs485_poll(void);

/**
 * Enqueue bytes to send. Buffer is copied so it is safe to be reused.
 */ 
void rs485_write(const uint8_t* data, uint8_t size);

/**
 * Read data, if available.
 */
_Bool rs485_read(uint8_t* data, uint8_t size);

/**
 * Get count of available bytes in the read buffer
 */
uint8_t rs485_readAvail();

/**
 * Get count of available bytes in the write buffer
 */
uint8_t rs485_writeAvail(void);

// The following timings are required to avoid the two-wire RS485 line to remain floating, potentially
// triggering frame errors. The line will be driven low by two stations at the same time.
// The total time should be however less than 3.5 characters to avoid triggering timeout errors.

// TICKS_PER_SECOND = 3906 on PIC16 @4MHz
// TICKS_PER_SECOND = 24414 on PIC18 @25MHz
// CHAR_PER_SECONDS = (BAUD / 11 (9+1+1)) = 1744 (round down) = 0.57ms
#define CHAR_PER_SECONDS (uint32_t)((RS485_BAUD - 11) / 11)
// 2 ticks per byte, but let's do 3 (round up) for PIC16 @4MHz
// 14 ticks per byte (round up) on PIC18 @25MHz
#define TICKS_PER_CHAR (TICK_TYPE)((TICKS_PER_SECOND + CHAR_PER_SECONDS) / CHAR_PER_SECONDS)

// Time to wait before transmitting after channel switched from RX to TX.
#define START_TRANSMIT_TIMEOUT (TICK_TYPE)(TICKS_PER_CHAR * 2)
// time to wait before releasing the channel from transmit to receive
// but let's wait an additional full byte since USART is free when still transmitting the last byte.
#define DISENGAGE_CHANNEL_TIMEOUT (TICK_TYPE)(TICKS_PER_CHAR * (1.5 + 1))

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
    RS485_LINE_WAIT_FOR_START_TRANSMIT,
    // Read and discard characters until frame end
    RS485_LINE_RX_SKIP
} RS485_LINE_STATE;
extern RS485_LINE_STATE rs485_state;

#ifdef __cplusplus
}
#endif

#endif	/* RS485_H */

