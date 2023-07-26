#ifndef RS485_H
#define	RS485_H

#include "configuration.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * High-level generic RS485 operation module virtualization. 
 * It basically encapsulate a 8-bit UART with additional direction bit (usually RTS on RS485 USB dongles).
 * To ease XC8 compiler, it uses a shared static read/write buffer without intermediate buffering. This will require
 * strict polling for flushing the buffer in real-time.
 */

/**
 * Initialize asynchronous mode, but only half-duplex is used
 */
void rs485_init();

/**
 * When the bus is engaged and a packet is transiting, poll should be called with 
 * a frequency at least 0.5 character period (e.g. 280us on 19200 baud).
 * When the bus is idle instead, the poll can be less intensive, depending 
 * on the HW buffer size of the UART in use (e.g. 2 characters, so 1ms on 19200 baud, on a PIC MCU, or even 
 * slower on a more buffered UART, but at least able to detect a message before his end).
 * Returns `true` if the bus is active (so fast poll is required).
 */
_Bool rs485_poll();

/**
 * Set to true when the line is not active from more than 3.5 characters (ModBus mark condition)
 */
extern _Bool rs485_isMarkCondition;

/**
 * The whole buffer. `RS485_BUF_SIZE` should be at least 16 bytes.
 * The buffer data should not be accessed until operation is completed.
 */
extern uint8_t rs485_buffer[RS485_BUF_SIZE];

/**
 * Start writing the data in the `rs485_buffer`.
 * `size` is the number of bytes valid in the buffer to write.
 */ 
void rs485_write(uint8_t size);

/**
 * Discard `count` bytes from the read buffer
 */
void rs485_discard(uint8_t count);

/**
 * Get count of available bytes in the read `rs485_buffer`
 */
uint8_t rs485_readAvail();

/**
 * Check if the buffer contains data still to be sent
 */
_Bool rs485_writeInProgress();

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
// So the total time between request and response is MARK_CONDITION_TIMEOUT + START_TRANSMIT_TIMEOUT = 3.5
#define START_TRANSMIT_TIMEOUT (TICK_TYPE)(TICKS_PER_CHAR * 2)

// Time to wait before releasing the channel from transmit to receive
// but let's wait an additional full byte since UART is free when still transmitting the last byte.
#define DISENGAGE_CHANNEL_TIMEOUT (TICK_TYPE)(TICKS_PER_CHAR * (2 + 1))

// The mark condition that separates messages in Modbus is actually 3.5 characters
// However to implement such restricted timing and to guarantee the correct overlap
// between master drive and slave line drive (to avoid glitches) the mark condition is set 
// to slightly more than 1 character to allow the correct handshake. The master uses 2
// character as DISENGAGE_CHANNEL_TIMEOUT
#define MARK_CONDITION_TIMEOUT (TICK_TYPE)(TICKS_PER_CHAR * 1.5)

/**
 * State of the RS485 line
 * @internal
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
} RS485_LINE_STATE;
extern RS485_LINE_STATE rs485_state;

#ifdef __cplusplus
}
#endif

#endif	/* RS485_H */

