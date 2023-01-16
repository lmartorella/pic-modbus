#include <stdbool.h>
#include <stdint.h>
#include "net/rs485.h"
#include "net/timers.h"
#include "net/uart.h"

RS485_LINE_STATE rs485_state;

// Circular buffer
static uint8_t s_buffer[RS485_BUF_SIZE];
// Pointer of the writing head
static uint8_t s_writePtr;
// Pointer of the reading head (if = write ptr, no bytes avail)
static uint8_t s_readPtr;
// Once this is set, it skip reading in the buffer until mark condition is detected.
// @internal
_Bool rs485_frameError;
_Bool rs485_isMarkCondition;

static uint8_t _rs485_readAvail() {
    return (uint8_t)(((uint8_t)(s_writePtr - s_readPtr)) % RS485_BUF_SIZE);
}

static uint8_t _rs485_writeAvail() {
    return (uint8_t)(((uint8_t)(s_readPtr - s_writePtr - 1)) % RS485_BUF_SIZE);
}

// Set at the beginning of states RS485_LINE_TX_DISENGAGE, RS485_LINE_WAIT_FOR_START_TRANSMIT
// In RS485_LINE_RX mode, it is set for every character received to detect mark condition
static TICK_TYPE s_lastTick;

static void rs485_startRead() {
    // Disable writing
    uart_disable_tx();

    // Disable RS485 driver
    uart_receive();
    rs485_state = RS485_LINE_RX;
    rs485_frameError = false;
    rs485_isMarkCondition = true;

    // Reset circular buffer
    s_readPtr = s_writePtr = 0;

    // Enable UART receiver
    uart_enable_rx();
}

void rs485_init() {
    uart_init();

    s_writePtr = s_readPtr = 0;
    s_lastTick = timers_get();
    
    rs485_startRead();
}

uint8_t rs485_readAvail() {
    if (rs485_state == RS485_LINE_RX) {
        return _rs485_readAvail();
    } else {
        return 0;
    }
}

uint8_t rs485_writeAvail() {
    if (rs485_state != RS485_LINE_RX) {
        return _rs485_writeAvail();
    } else {
        // Can switch to TX and have full buffer
        return RS485_BUF_SIZE - 1;
    }
}

// Feed more data, read at read pointer and then increase
// and re-enable interrupts now
static void writeByte() {
    uart_write(s_buffer[s_readPtr]);
    s_readPtr = (s_readPtr + 1) % RS485_BUF_SIZE;
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
    }

    switch (rs485_state) {
        case RS485_LINE_TX: {
            // Empty TX buffer? Check for more data
            while (uart_tx_fifo_empty()) {
                if (_rs485_readAvail() > 0) {
                    // Feed more data, read at read pointer and then increase
                    writeByte();
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
                uint8_t data;
                UART_RX_MD md;

                uart_read(&data, &md);

                if (md.overrunErr) {
                    // Not enough fast polling, reboot
                    fatal("U.OER");
                }
                if (md.frameErr) {
                    rs485_frameError = true;
                }
                
                // Only read data if not in skip mode
                if (!rs485_frameError) {
                    s_buffer[s_writePtr] = data;
                    s_writePtr = (s_writePtr + 1) % RS485_BUF_SIZE;

                    if (s_writePtr == s_readPtr) {
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

void rs485_write(const void* data, uint8_t size) {
    // Abort reader, if in progress
    if (rs485_state == RS485_LINE_RX) {
        // Truncate reading
        uart_disable_rx();
        s_readPtr = s_writePtr = 0;

        // Enable UART transmit.
        uart_enable_tx();

        // Engage
        rs485_state = RS485_LINE_WAIT_FOR_START_TRANSMIT;
        s_lastTick = timers_get();
        // Enable RS485 driver
        uart_transmit();
    } else if (rs485_state == RS485_LINE_TX_DISENGAGE) {
        // Re-convert it to tx without additional delays
        rs485_state = RS485_LINE_TX;
    }

    if (size > _rs485_writeAvail()) {
        // Overflow error
        fatal("U.wov");
    }
    
    // Copy to buffer
    while (size > 0) {
        s_buffer[s_writePtr] = *((const uint8_t*)data);
        data = (const uint8_t*)data + 1;
        s_writePtr = (s_writePtr + 1) % RS485_BUF_SIZE;
        size--;
    }
}

_Bool rs485_read(void* data, uint8_t size) {
    if (rs485_state != RS485_LINE_RX) { 
        if (rs485_state == RS485_LINE_TX || rs485_state == RS485_LINE_WAIT_FOR_START_TRANSMIT) {
            // Break all
            rs485_state = RS485_LINE_TX_DISENGAGE;
            s_lastTick = timers_get();
        }
        return false;
    } else {
        _Bool ret = false;
        // Active? Read immediately.
        if (_rs485_readAvail() >= size) {
            ret = true;
            while (size > 0) {
                *((uint8_t*)data) = s_buffer[s_readPtr];
                data = (uint8_t*)data + 1;
                s_readPtr = (s_readPtr + 1) % RS485_BUF_SIZE;
                size--;
            }
        }
        return ret;
    }
}

void rs485_discard() {
    s_readPtr = s_writePtr;
}