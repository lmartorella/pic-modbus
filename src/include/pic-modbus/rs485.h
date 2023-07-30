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
extern _Bool rs485_packet_end;

/**
 * The whole buffer. `RS485_BUF_SIZE` should be at least 16 bytes.
 * The buffer data should not be accessed until operation is completed.
 */
extern uint8_t rs485_buffer[RS485_BUF_SIZE];

/**
 * Start writing the data in the `rs485_buffer`. The packet can be partial.
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
uint8_t rs485_read_avail();

/**
 * Check if the buffer contains data still to be sent
 */
_Bool rs485_write_in_progress();

/**
 * In receive state?
 */
_Bool rs485_in_receive_state();

#ifdef __cplusplus
}
#endif

#endif	/* RS485_H */

