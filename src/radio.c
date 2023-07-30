#ifdef _CONF_PACKET_RADIO

#include <stdint.h>
#include <stdio.h>
#include "configuration.h"
#include "pic-modbus/hw/lt8920.h"
#include "pic-modbus/radio.h"
#include "pic-modbus/sys.h"

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

static uint8_t readAvail;
static _Bool packetReady;
/**
 * The whole buffer. `RADIO_BUF_SIZE` should be at least 16 bytes.
 * The buffer data should not be accessed until operation is completed.
 */
uint8_t radio_buffer[RADIO_BUF_SIZE];
_Bool radio_packet_end = false;

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
    lt8920_init();
    lt8920_reset();
    radio_start_read();
}

#ifdef _DEBUG
static void _debug_fifo(const char* msg) {
    lt8920_read_fifo_ctrl();
    lt8920_get_status();
    printf("FIFO ptr: W: %d, R: %d, FIFO_FLAG: %d (%s)\n",
        lt8920_registers.fifo_ctrl.b.FIFO_WR_PTR,
        lt8920_registers.fifo_ctrl.b.FIFO_RD_PTR,
        lt8920_registers.status.b.FIFO_FLAG,
        msg);
}
#endif

static void radio_read_packet() {
#if _DEBUG
    _debug_fifo("begin");
#endif
    
#ifdef LT8920_FIRST_BYTE_AS_LENGTH
    // 1st byte of payload used for packet length, for the LT8920 framer
    readAvail = lt8920_read_fifo();
    _debug_fifo("after 1 byte read");

    uint8_t pos = 0;
    while (pos < readAvail) {
        radio_buffer[pos++] = lt8920_read_fifo();
    }
    _debug_fifo("end");
#else
    readAvail = 0;
    while (lt8920_registers.fifo_ctrl.b.FIFO_WR_PTR > lt8920_registers.fifo_ctrl.b.FIFO_RD_PTR) {
        radio_buffer[readAvail++] = lt8920_read_fifo();
#if _DEBUG
        _debug_fifo("looping");
#endif
    }
#endif

    packetReady = true;
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
#ifdef _DEBUG
        if (lt8920_registers.status.b.FIFO_FLAG) {
            printf("FIFO flag set\n");
        }
        if (lt8920_registers.status.b.SYNCWORD_RECV) {
            printf("SYNCWORD_RECV flag set\n");
        }
#endif
        // In RX mode, PKT_FLAG means that a packet arrived
        if (lt8920_registers.status.b.PKT_FLAG) {
#ifdef _DEBUG
            if (lt8920_registers.status.b.PKT_FLAG) {
                printf("PKT_FLAG set\n");
            }
#endif
            if (lt8920_registers.status.b.CRC_ERROR || lt8920_registers.status.b.FEC23_ERROR) {
#ifdef _DEBUG
                printf("CRC error\n");
#endif
                // Discard the buffer
                radio_start_read();
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
void radio_write(uint8_t size) {
    if (radio_state != RADIO_LINE_TX) {
        lt8920_disable_rx_tx();
        lt8920_flush_rx_tx();
#if _DEBUG
        _debug_fifo("begin");
#endif
    }

#ifdef LT8920_FIRST_BYTE_AS_LENGTH
    #error Unsupported
#endif
    uint8_t pos = 0;
    while (pos < size) {
        lt8920_write_fifo(radio_buffer[pos++]);
    }
}

void radio_write_end() {
    lt8920_enable_tx();
    radio_state = RADIO_LINE_TX;

#ifndef LT8920_FIRST_BYTE_AS_LENGTH
    // FW_TERM_TX = 0 and PACK_LENGTH_EN = 0: TX stops only when TX_EN is set to zero. No 1st byte of payload used for packet length.
    // do {
    //     _debug_fifo("looping");
    // } while (lt8920_registers.fifo_ctrl.b.FIFO_RD_PTR < lt8920_registers.fifo_ctrl.b.FIFO_WR_PTR);

    lt8920_disable_rx_tx();
#else
    _debug_fifo("end");
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

_Bool radio_in_receive_state() {
    return radio_state == RADIO_LINE_RX;
}


/**
 * TODO: remove
 */
void radio_discard(uint8_t count) {
    if (count > readAvail || radio_state != RADIO_LINE_RX) {
        sys_fatal(EXC_CODE_BUF_DISCARD_MISMATCH);
    }
    for (uint8_t i = 0; i < count; i++) {
        radio_buffer[i] = radio_buffer[i + count];
    }
}

#endif