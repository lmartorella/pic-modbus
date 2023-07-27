#ifndef RS485_H
#define	RS485_H

#include "configuration.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * High-level generic LT8920 radio module operation virtualization. 
 * To ease XC8 compiler, it uses a shared static read/write buffer without intermediate buffering. This will require
 * strict polling for flushing the buffer in real-time.
 */

/**
 * Initialize asynchronous mode, but only half-duplex is used
 */
void lt8920_init();

/**
 * Returns `true` if the bus is active (so fast poll is required).
 */
_Bool lt8920_poll();

/**
 * The whole buffer. `LT8920_BUF_SIZE` should be at least 16 bytes.
 * The buffer data should not be accessed until operation is completed.
 */
extern uint8_t lt8920_buffer[LT8920_BUF_SIZE];

/**
 * Write a complete packet, found in the `lt8920_buffer`.
 * `size` is the number of bytes valid in the buffer to write.
 */ 
extern void lt8920_write_packet(uint8_t size);

/**
 * Discard `count` bytes from the read buffer
 */
extern void lt8920_discard(uint8_t count);

/**
 * Get count of available bytes in the read `lt8920_buffer`
 */
extern uint8_t lt8920_readAvail();

/**
 * Check if the buffer contains data still to be sent
 */
extern _Bool lt8920_writeInProgress();

#ifdef __cplusplus
}
#endif

#endif	/* RS485_H */
