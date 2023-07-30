#ifndef _RADIO_H
#define	_RADIO_H

#include "configuration.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * High-level generic packet-radio module operation virtualization. 
 * To ease XC8 compiler, it uses a shared static read/write buffer without intermediate buffering. This will require
 * strict polling for flushing the buffer in real-time.
 */

/**
 * Initialize asynchronous mode, but only half-duplex is used
 */
void radio_init();

/**
 * Returns `true` if the bus is active (so fast poll is required).
 */
_Bool radio_poll();

/**
 * The whole buffer. `RADIO_BUF_SIZE` should be at least 16 bytes.
 * The buffer data should not be accessed until operation is completed.
 */
extern uint8_t radio_buffer[RADIO_BUF_SIZE];

/**
 * Start/continue writing a packet, data found in the `radio_buffer[0]`.
 * `size` is the number of bytes valid in the buffer to write.
 */ 
extern void radio_write(uint8_t size);

extern void radio_write_end();

/**
 * Is packet ready (even if empty)
 */
extern _Bool radio_packet_ready();

/**
 * Get count of available bytes in the read `radio_buffer`
 */
extern uint8_t radio_read_avail();

/**
 * Check if the buffer contains data still to be sent
 */
extern _Bool radio_write_in_progress();

extern _Bool radio_in_receive_state();

/**
 * TODO: remove
 */
extern void radio_discard(uint8_t count);

extern _Bool radio_packet_end;

#ifdef __cplusplus
}
#endif

#endif
