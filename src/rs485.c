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

static uint8_t s_frameErrors;

static uint8_t _rs485_readAvail() {
    return (uint8_t)(((uint8_t)(s_writePtr - s_readPtr)) % RS485_BUF_SIZE);
}

static uint8_t _rs485_writeAvail() {
    return (uint8_t)(((uint8_t)(s_readPtr - s_writePtr - 1)) % RS485_BUF_SIZE);
}

static TICK_TYPE s_lastTick;

static void rs485_startRead() {
    if (rs485_state == RS485_LINE_TX || rs485_state == RS485_LINE_WAIT_FOR_START_TRANSMIT) {
        // Break all
        rs485_state = RS485_LINE_TX_DISENGAGE;
        s_lastTick = timers_get();
        return;
    }
    
    // Disable writing
    uart_disable_tx();

    // Disable RS485 driver
    uart_receive();
    rs485_state = RS485_LINE_RX;

    // Reset circular buffer
    s_readPtr = s_writePtr = 0;

    // Enable UART receiver
    uart_enable_rx();
}

void rs485_init() {
    s_frameErrors = 0;

    uart_init();

    s_writePtr = s_readPtr = 0;
    s_lastTick = timers_get();
    
    rs485_state = RS485_LINE_RX;
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
        rs485_state = RS485_LINE_RX;
        rs485_startRead();
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
                    s_lastTick = timers_get();
                    rs485_state = RS485_LINE_TX_DISENGAGE;
                    return true;
                }
            };
            // Wait for a character to be written: slow timer
            return false;
        } 
        case RS485_LINE_RX: 
        case RS485_LINE_RX_SKIP: {
            // Data received
            while (!uart_rx_fifo_empty()) {                
                uint8_t data;
                UART_RX_MD md;

                uart_read(&data, &md);
                
                if (md.overrunErr) {
                    fatal("U.OER");
                    return false;
                }
                if (md.frameErr) {
                    s_frameErrors++;
                    rs485_state = RS485_LINE_RX_SKIP;
                }
                
                // Only read data if not in skip mode
                if (rs485_state != RS485_LINE_RX_SKIP) {
                    s_buffer[s_writePtr] = data;
                    s_writePtr = (s_writePtr + 1) % RS485_BUF_SIZE;

                    if (s_writePtr == s_readPtr) {
                        // Overflow error
                        fatal("U.rov");
                    }
                }
            };
            // Wait for a character to be read: slow timer
            return false;
        }
    }

    // More granularity required for Timer-based activities 
    return true;
}

void rs485_write(const uint8_t* data, uint8_t size) {
    // Abort reader, if in progress
    if (rs485_state == RS485_LINE_RX || rs485_state == RS485_LINE_RX_SKIP) {
        // Truncate reading
        uart_disable_rx();
        s_readPtr = s_writePtr = 0;

        // Enable UART transmit.
        uart_enable_tx();

        // Engage
        rs485_state = RS485_LINE_WAIT_FOR_START_TRANSMIT;
        // Enable RS485 driver
        uart_transmit();
        s_lastTick = timers_get();
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
        s_buffer[s_writePtr] = *(data++);
        s_writePtr = (s_writePtr + 1) % RS485_BUF_SIZE;
        size--;
    }
}

_Bool rs485_read(uint8_t* data, uint8_t size) {
    if (rs485_state != RS485_LINE_RX) { 
        rs485_startRead();
        return false;
    } else {
        _Bool ret = false;
        // Active? Read immediately.
        if (_rs485_readAvail() >= size) {
            ret = true;
            while (size > 0) {
                *(data++) = s_buffer[s_readPtr];
                s_readPtr = (s_readPtr + 1) % RS485_BUF_SIZE;
                size--;
            }
        }
        return ret;
    }
}
