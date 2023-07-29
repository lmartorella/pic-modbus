#include <stdint.h>
#include "hw.h"
#include "lt8920.h"
#include "radio.h"

/**
 * State of the LT8920 line
 */
typedef enum {
    // Receiving, all OK
    RADIO_LINE_RX,
    // Transmitting, data
    RADIO_LINE_TX
} RADIO_LINE_STATE;
static RADIO_LINE_STATE radio_state;

static int readAvail;
/**
 * The whole buffer. `RADIO_BUF_SIZE` should be at least 16 bytes.
 * The buffer data should not be accessed until operation is completed.
 */
uint8_t radio_buffer[RADIO_BUF_SIZE];

static void radio_start_read() {
    lt8920_disable_rx_tx();
    lt8920_flush_rx();
    lt8920_enable_rx();

    radio_state = RADIO_LINE_RX;
    readAvail = 0;
}

/**
 * Setup connection
 */
void radio_init() {
    lt8920_reset();
    radio_start_read();
}

static void radio_read_packet() {
    readAvail = lt8920_read_fifo();
    uint8_t pos = 0;
    while (pos < readAvail) {
        radio_buffer[pos++] = lt8920_read_fifo();
    }
}

static void radio_discard() {
    lt8920_read_fifo();
}

/**
 * Returns `true` if the bus is active (so fast poll is required).
 */
_Bool radio_poll() {
    lt8920_get_status();
    if (radio_state == RADIO_LINE_TX) {
        if (lt8920_registers.status.b.PKT_FLAG && !lt8920_registers.status.b.FIFO_FLAG) {
            // Go back in receive mode, TX frame sent
            radio_start_read();
        } else {
            // Poll fast
            return true;
        }
    } else {
        if (lt8920_registers.status.b.PKT_FLAG) {
            if (lt8920_registers.status.b.CRC_ERROR || lt8920_registers.status.b.FEC23_ERROR) {
                radio_discard();
            } else {
                // Data arrived, read data
                radio_read_packet();
            }
        }
    }
    return false;
}

/**
 * Start writing the data in the `radio_buffer`.
 * `size` is the number of bytes valid in the buffer to write.
 */ 
void radio_write_packet(uint8_t size) {
    lt8920_disable_rx_tx();
    lt8920_flush_tx();

    lt8920_write_fifo(size);
    uint8_t pos = 0;
    while (pos < size) {
        lt8920_write_fifo(radio_buffer[pos++]);
    }

    lt8920_enable_tx();

    radio_state = RADIO_LINE_TX;
}

/**
 * Get count of available bytes in the read `radio_buffer`
 */
uint8_t radio_readAvail() {
    return readAvail;
}

/**
 * Check if the buffer contains data still to be sent
 */
_Bool radio_writeInProgress() {
    return radio_state == RADIO_LINE_TX;
}
