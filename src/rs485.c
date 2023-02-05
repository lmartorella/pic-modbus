#include <stdbool.h>
#include <stdint.h>
#include "net/crc.h"
#include "net/rs485.h"
#include "net/timers.h"
#include "net/uart.h"

RS485_LINE_STATE rs485_state;
uint8_t rs485_buffer[RS485_BUF_SIZE];

// Pointer of the read/writing head
static uint8_t s_bufferPtr;
// Size of the valid to-be-written data in the buffer
static uint8_t s_writeDataSize;
// Once this is set, it skip reading in the buffer until mark condition is detected.
// @internal
_Bool rs485_frameError;
_Bool rs485_isMarkCondition;

// Set at the beginning of states RS485_LINE_TX_DISENGAGE, RS485_LINE_WAIT_FOR_START_TRANSMIT
// In RS485_LINE_RX mode, it is set for every character received to detect mark condition
static TICK_TYPE s_lastTick;

static void rs485_startRead() {
    // Disable RS485 driver
    uart_receive();
    rs485_state = RS485_LINE_RX;
    rs485_frameError = false;
    rs485_isMarkCondition = true;
    crc_reset();

    // Reset circular buffer
    s_bufferPtr = 0;
}

void rs485_init() {
    uart_init();
    s_lastTick = timers_get();
    rs485_startRead();
}

uint8_t rs485_readAvail() {
    if (rs485_state == RS485_LINE_RX) {
        return s_bufferPtr;
    } else {
        return 0;
    }
}

_Bool rs485_writeInProgress() {
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
        rs485_isMarkCondition = true;
        crc_reset();
    }

    switch (rs485_state) {
        case RS485_LINE_TX: {
            // Empty TX buffer? Check for more data
            while (uart_tx_fifo_empty()) {
                if (rs485_writeInProgress()) {
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
        } 
        case RS485_LINE_RX: {
            // Data received
            _Bool haveData = false;
            while (!uart_rx_fifo_empty()) {
                haveData = true;
                uint8_t ch;
                UART_RX_MD md;

                uart_read(&ch, &md);

                if (md.overrunErr) {
                    // Not enough fast polling, reboot
                    fatal("U.OER");
                }
                if (md.frameErr) {
                    rs485_frameError = true;
                }
                
                // Only read data if not in skip mode
                if (!rs485_frameError) {
                    rs485_buffer[s_bufferPtr++] = ch;
                    if (s_bufferPtr > RS485_BUF_SIZE) {
                        // Overflow error
                        fatal("U.rov");
                    }
                }
            }
            if (haveData) {
                s_lastTick = timers_get();
                rs485_isMarkCondition = false;
            }

            // Wait for a character to be read: slow timer
            return false;
        }
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
    s_bufferPtr = 0;
    s_writeDataSize = size;
}

void rs485_read() {
    if (rs485_state == RS485_LINE_TX || rs485_state == RS485_LINE_WAIT_FOR_START_TRANSMIT) {
        // Break all
        rs485_state = RS485_LINE_TX_DISENGAGE;
        s_lastTick = timers_get();
        crc_reset();
    }
}

void rs485_discard(uint8_t count) {
    if (count != s_bufferPtr) {
        fatal("U.dov");
    }
    for (uint8_t i = 0; i < count; i++) {
        crc_update(rs485_buffer[i]);
    }
    s_bufferPtr = 0;
}
