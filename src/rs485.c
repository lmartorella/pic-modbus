#ifdef _CONF_RS485

#include "pic-modbus/crc.h"
#include "pic-modbus/rs485.h"
#include "pic-modbus/sys.h"
#include "pic-modbus/timers.h"
#include "pic-modbus/hw/uart.h"

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
} RS485_LINE_STATE;
static RS485_LINE_STATE rs485_state;

uint8_t rs485_buffer[RS485_BUF_SIZE];

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

// Pointer of the read/writing head
static uint8_t s_bufferPtr;
// Size of the valid to-be-written data in the buffer
static uint8_t s_writeDataSize;
// Once this is set, it skip reading in the buffer until mark condition is detected.
// extern for UTs
static _Bool rs485_frameError;
_Bool rs485_packet_end;

// Set at the beginning of states RS485_LINE_TX_DISENGAGE, RS485_LINE_WAIT_FOR_START_TRANSMIT
// In RS485_LINE_RX mode, it is set for every character received to detect mark condition
static TICK_TYPE s_lastTick;

static void rs485_startRead() {
    // Disable RS485 driver
    uart_receive();
    rs485_state = RS485_LINE_RX;
    rs485_frameError = false;
    rs485_packet_end = true;
    crc_reset();

    // Reset circular buffer
    s_bufferPtr = 0;
}

void rs485_init() {
    uart_init();
    s_lastTick = timers_get();
    rs485_startRead();
}

uint8_t rs485_read_avail() {
    if (rs485_state == RS485_LINE_RX) {
        return s_bufferPtr;
    } else {
        return 0;
    }
}

_Bool rs485_write_in_progress() {
    if (rs485_state != RS485_LINE_RX) {
        return s_bufferPtr < s_writeDataSize;
    } else {
        // Can switch to TX and have full buffer
        return false;
    }
}

/**
 * Returns `true` if the bus is active (so fast poll is required).
 */
_Bool rs485_poll() {
    TICK_TYPE elapsed = timers_get() - s_lastTick;

    if (rs485_state == RS485_LINE_WAIT_FOR_START_TRANSMIT && elapsed >= START_TRANSMIT_TIMEOUT) {
        // Go in TX mode
        rs485_state = RS485_LINE_TX;
    } else if (rs485_state == RS485_LINE_TX_DISENGAGE && elapsed >= DISENGAGE_CHANNEL_TIMEOUT) {
        // Detach TX line
        rs485_startRead();
    } else if (rs485_state == RS485_LINE_RX && elapsed >= MARK_CONDITION_TIMEOUT) {
        rs485_packet_end = true;
        rs485_frameError = false;
        crc_reset();
    }

    if (rs485_state == RS485_LINE_TX) {
        // Empty TX buffer? Check for more data
        while (uart_tx_fifo_empty()) {
            if (rs485_write_in_progress()) {
                // Feed more data, read at read pointer and then increase
                uint8_t ch = rs485_buffer[s_bufferPtr++];
                uart_write(ch);
                crc_update(ch);
            } else {
                // NO MORE data to transmit
                // goto first phase of tx end
                rs485_state = RS485_LINE_TX_DISENGAGE;
                s_lastTick = timers_get();
                return true;
            }
        };
        // Wait for a character to be written: slow timer
        return false;
    } else if (rs485_state == RS485_LINE_RX) {
        // Data received
        _Bool haveData = false;
        while (!uart_rx_fifo_empty()) {
            haveData = true;

            uart_read();

            if (uart_lastCh.errs.OERR) {
                // Not enough fast polling, reboot
                sys_fatal(EXC_CODE_RS485_READ_UNDERRUN);
            }
            if (uart_lastCh.errs.FERR) {
                rs485_frameError = true;
            }

            // Only read data if not in skip mode
            if (!rs485_frameError) {
                rs485_buffer[s_bufferPtr++] = uart_lastCh.data;
                if (s_bufferPtr > RS485_BUF_SIZE) {
                    // Overflow error
                    sys_fatal(EXC_CODE_RS485_READ_OVERRUN);
                }
            }
        }

        if (haveData) {
            // Mark the last byte received timestamp
            s_lastTick = timers_get();
            rs485_packet_end = false;
        }

        // Wait for a character to be read: slow timer
        return false;
    }

    // More granularity required for Timer-based activities 
    return true;
}

void rs485_write(uint8_t size) {
    // Abort reader, if in progress
    if (rs485_state == RS485_LINE_RX) {
        // Enable RS485 driver
        uart_transmit();

        crc_reset();

        // Engage
        rs485_state = RS485_LINE_WAIT_FOR_START_TRANSMIT;
        s_lastTick = timers_get();
    } else if (rs485_state == RS485_LINE_TX_DISENGAGE) {
        // Re-convert it to tx without additional delays
        rs485_state = RS485_LINE_TX;
    }
    rs485_packet_end = false;
    s_bufferPtr = 0;
    s_writeDataSize = size;
}

void rs485_discard(uint8_t count) {
    if (count != s_bufferPtr || rs485_state != RS485_LINE_RX) {
        sys_fatal(EXC_CODE_BUF_DISCARD_MISMATCH);
    }
    for (uint8_t i = 0; i < count; i++) {
        crc_update(rs485_buffer[i]);
    }
    s_bufferPtr = 0;
}

_Bool rs485_in_receive_state() {
    return rs485_state == RS485_LINE_RX;
}

#endif
