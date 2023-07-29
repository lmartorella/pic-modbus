#include <stdint.h>
#include <stdio.h>
#include "configuration.h"
#include "hw.h"
#include "lt8920.h"
#include "radio.h"

/**
 * State of the LT8920 line
 */
typedef enum {
    // Receiving, all OK
    RADIO_LINE_RX,
    // Transmitting, data, FIFO is still opened
    RADIO_LINE_TX,
    // TX ending packet, FIFO is closed, waiting for PKT
    RADIO_LINE_TX_WAITING_FOR_END
} RADIO_LINE_STATE;
static RADIO_LINE_STATE radio_state;

static int readAvail;
static _Bool packetReady;
/**
 * The whole buffer. `RADIO_BUF_SIZE` should be at least 16 bytes.
 * The buffer data should not be accessed until operation is completed.
 */
uint8_t radio_buffer[RADIO_BUF_SIZE];

static void radio_start_read() {
    lt8920_disable_rx_tx();
    lt8920_flush_rx_tx();
    // This will reset PKT
    lt8920_read_fifo();
    lt8920_enable_rx();

    radio_state = RADIO_LINE_RX;
    readAvail = 0;
    packetReady = false;
}

/**
 * Setup connection
 */
void radio_init() {
    lt8920_reset();
    radio_start_read();
}

static void radio_read_packet() {
    lt8920_read_fifo_ctrl();
    printf("FIFO ptr: W: %d, R: %d\n", lt8920_registers.fifo_ctrl.b.FIFO_WR_PTR, lt8920_registers.fifo_ctrl.b.FIFO_RD_PTR);

#ifdef LT8920_FIRST_BYTE_AS_LENGTH
    // 1st byte of payload used for packet length, for the LT8920 framer
    readAvail = lt8920_read_fifo();
    lt8920_read_fifo_ctrl();
    printf("FIFO ptr: W: %d, R: %d (after 1 byte read)\n", lt8920_registers.fifo_ctrl.b.FIFO_WR_PTR, lt8920_registers.fifo_ctrl.b.FIFO_RD_PTR);

    uint8_t pos = 0;
    while (pos < readAvail) {
        radio_buffer[pos++] = lt8920_read_fifo();
    }
#else
    readAvail = 0;
    while (lt8920_registers.status.b.FIFO_FLAG) {
        radio_buffer[readAvail++] = lt8920_read_fifo();
        lt8920_get_status();
    }
#endif

    lt8920_read_fifo_ctrl();
    printf("FIFO ptr: W: %d, R: %d (after packet read)\n", lt8920_registers.fifo_ctrl.b.FIFO_WR_PTR, lt8920_registers.fifo_ctrl.b.FIFO_RD_PTR);

    packetReady = true;
}

static void radio_discard() {
    radio_start_read();
}

/**
 * Returns `true` if the bus is active (so fast poll is required).
 */
_Bool radio_poll() {
    lt8920_get_status();
    if (radio_state == RADIO_LINE_TX_WAITING_FOR_END) {
        if (lt8920_registers.status.b.PKT_FLAG) {
            // Go back in receive mode, TX frame sent
            radio_start_read();
        } else {
            // Poll fast
            return true;
        }
    } else if (radio_state == RADIO_LINE_RX) {
        if (lt8920_registers.status.b.FIFO_FLAG) {
            printf("FIFO flag set\n");
        }
        // if (lt8920_registers.status.b.FRAMER_ST != 18 && lt8920_registers.status.b.FRAMER_ST != 0) {
        //     printf("FRAMER_ST state set to %d\n", lt8920_registers.status.b.FRAMER_ST);
        // }
        if (lt8920_registers.status.b.SYNCWORD_RECV) {
            printf("SYNCWORD_RECV flag set\n");
        }
        // In RX mode, PKT_FLAG means that a packet arrived
        if (lt8920_registers.status.b.PKT_FLAG) {
            if (lt8920_registers.status.b.PKT_FLAG) {
                printf("PKT_FLAG set\n");
            // } else {
            //     printf("FRAMER_ST status IDLE\n");
            }
            if (lt8920_registers.status.b.CRC_ERROR || lt8920_registers.status.b.FEC23_ERROR) {
                printf("CRC error\n");
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
    lt8920_flush_rx_tx();

    lt8920_read_fifo_ctrl();
    printf("FIFO ptr: W: %d, R: %d\n", lt8920_registers.fifo_ctrl.b.FIFO_WR_PTR, lt8920_registers.fifo_ctrl.b.FIFO_RD_PTR);

#ifdef LT8920_FIRST_BYTE_AS_LENGTH
    // 1st byte of payload used for packet length, for the LT8920 framer
    lt8920_write_fifo(size);
    lt8920_read_fifo_ctrl();
    printf("FIFO ptr: W: %d, R: %d (after 1 byte write)\n", lt8920_registers.fifo_ctrl.b.FIFO_WR_PTR, lt8920_registers.fifo_ctrl.b.FIFO_RD_PTR);
#endif
    uint8_t pos = 0;
    while (pos < size) {
        lt8920_write_fifo(radio_buffer[pos++]);
    }

    lt8920_read_fifo_ctrl();
    printf("FIFO ptr: W: %d, R: %d (end)\n", lt8920_registers.fifo_ctrl.b.FIFO_WR_PTR, lt8920_registers.fifo_ctrl.b.FIFO_RD_PTR);

    lt8920_enable_tx();
    radio_state = RADIO_LINE_TX;

#ifndef LT8920_FIRST_BYTE_AS_LENGTH
    // FW_TERM_TX = 0 and PACK_LENGTH_EN = 0: TX stops only when TX_EN is set to zero. No 1st byte of payload used for packet length.
    lt8920_disable_rx_tx();
#endif
    // Still in TX mode, waiting for packet to be completely sent
    radio_state = RADIO_LINE_TX_WAITING_FOR_END;
}

/**
 * Get count of available bytes in the read `radio_buffer`
 */
uint8_t radio_read_avail() {
    return readAvail;
}

_Bool radio_packet_ready() {
    return packetReady;
}

/**
 * Check if the buffer contains data still to be sent
 */
_Bool radio_write_in_progress() {
    return radio_state == RADIO_LINE_TX;
}
