#include "net/net.h"

/**
 * State of the RS485 line
 */
static enum {
    // Receiving, all OK
    RS485_LINE_RX,
    // End of transmitting, in disengage line period
    RS485_LINE_TX_DISENGAGE,
    // Transmitting, data
    RS485_LINE_TX,
    // After TX engaged, wait before transmitting
    RS485_LINE_WAIT_FOR_START_TRANSMIT,
    // Read and discard characters until frame end
    RS485_LINE_RX_SKIP
} s_state;

// Circular buffer
static uint8_t s_buffer[RS485_BUF_SIZE];
// Pointer of the writing head
static uint8_t* s_writePtr;
// Pointer of the reading head (if = write ptr, no bytes avail)
static uint8_t* s_readPtr;

static void _rs485_readAvail() {
    return (uint8_t)(((uint8_t)(s_writePtr - s_readPtr)) % RS485_BUF_SIZE);
}

static void _rs485_writeAvail() {
    return (uint8_t)(((uint8_t)(s_readPtr - s_writePtr - 1)) % RS485_BUF_SIZE);
}

static TICK_TYPE s_lastTick;

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
#define START_TRANSMIT_TIMEOUT (TICK_TYPE)(TICKS_PER_CHAR * 2)
// time to wait before releasing the channel from transmit to receive
// but let's wait an additional full byte since USART is free when still transmitting the last byte.
#define DISENGAGE_CHANNEL_TIMEOUT (TICK_TYPE)(TICKS_PER_CHAR * (1.5 + 1))

static void rs485_startRead(void);

void rs485_init() {
    uart_init();
    uart_receive();
       
    rs485_skipData = false;
    s_writePtr = s_readPtr = s_buffer;
    s_lastTick = timers_get();
    
    s_state = RS485_LINE_RX;
    rs485_startRead();
}

uint8_t rs485_readAvail() {
    if (s_state == RS485_LINE_RX) {
        return _rs485_readAvail();
    } else {
        return 0;
    }
}

uint8_t rs485_writeAvail() {
    if (s_state != RS485_LINE_RX) {
        return _rs485_writeAvail();
    } else {
        // Can switch to TX and have full buffer
        return RS485_BUF_SIZE - 1;
    }
}

// Feed more data, read at read pointer and then increase
// and re-enable interrupts now
static void writeByte() {
    uart_write(*(s_readPtr++));
    s_readPtr = s_readPtr % RS485_BUF_SIZE;
}

/**
 * Returns `true` if the bus is active (so fast poll is required).
 */
_Bool rs485_poll() {
    CLRWDT();

    TICK_TYPE elapsed = timers_get() - s_lastTick;
    switch (s_state) {
        case RS485_LINE_TX: {
            // Empty TX buffer? Check for more data
            while (uart_tx_fifo_empty()) {
                CLRWDT();
                if (_rs485_readAvail() > 0) {
                    // Feed more data, read at read pointer and then increase
                    writeByte();
                } else {
                    // NO MORE data to transmit
                    // goto first phase of tx end
                    s_lastTick = timers_get();
                    s_state = RS485_LINE_TX_DISENGAGE;
                    return true;
                }
            };
            return true;
        } 
        case RS485_LINE_TX: 
        case RS485_LINE_RX_SKIP: {
            // Data received
            while (!uart_rx_fifo_empty()) {
                CLRWDT();
                
                uint8_t data;
                UART_RX_MD md;

                uart_read(&data, &md);
                
                if (md.overrunErr) {
                    fatal("U.OER");
                    return false;
                }
                if (md.frameErr) {
                    s_frameErrors++;
                    s_state = RS485_LINE_RX_SKIP;
                }
                
                // Only read data if not in skip mode
                if (s_state != RS485_LINE_RX_SKIP) {
                    *(s_writePtr++) = data;
                    s_writePtr = s_writePtr % RS485_BUF_SIZE;
                }
            };
            return true;
        }
        case RS485_LINE_WAIT_FOR_START_TRANSMIT:
            if (elapsed >= START_TRANSMIT_TIMEOUT) {
                // Transmit
                s_state = RS485_LINE_TX;
                if (_rs485_readAvail() > 0) {
                    // Feed first byte
                    writeByte();
                }
            }
            break;
        case RS485_LINE_TX_DISENGAGE:
            if (elapsed >= DISENGAGE_CHANNEL_TIMEOUT) {
                // Detach TX line
                s_state = RS485_LINE_RX;
                rs485_startRead();
            }
            break;
    }
    return s_state != RS485_LINE_RX && s_state != RS485_LINE_RX_SKIP;
}

void rs485_write(const uint8_t* data, uint8_t size) { 
    rs485_master = 0;

    // Reset reader, if in progress
    if (s_state == RS485_LINE_RX || s_state == RS485_LINE_RX_SKIP) {
        // Truncate reading
        uart_disable_rx();
        s_readPtr = s_writePtr = s_buffer;

        // Enable UART transmit.
        uart_enable_tx();

        // Engage
        s_state = RS485_LINE_WAIT_FOR_START_TRANSMIT;
        // Enable RS485 driver
        uart_transmit();
        s_lastTick = timers_get();
    } else if (s_state == RS485_LINE_TX_DISENGAGE) {
        // Re-convert it to tx
        s_state = RS485_LINE_TX;
    }

    if (size > _rs485_writeAvail()) {
        // Overflow error
        fatal("U.ov");
    }
    
    // Copy to buffer
    while (size > 0) {
        *(s_writePtr++) = *(data++);
        s_writePtr = s_writePtr % RS485_BUF_SIZE;
        size--;
        CLRWDT();
    }
}

static void rs485_startRead() {
    CLRWDT();
    if (s_state == RS485_LINE_TX || s_state == RS485_LINE_WAIT_FOR_START_TRANSMIT) {
        // Break all
        s_state = RS485_LINE_TX_DISENGAGE;
        s_lastTick = timers_get();
        return;
    }
    
    // Disable writing
    uart_disable_tx();

    // Disable RS485 driver
    uart_receive();
    s_state = RS485_LINE_RX;

    // Reset circular buffer
    s_readPtr = s_writePtr = s_buffer;

    // Enable UART receiver
    uart_enable_rx();
}

__bit rs485_read(uint8_t* data, uint8_t size) {
    if (s_state != RS485_LINE_RX) { 
        rs485_startRead();
        return false;
    }
    else {
        static __bit ret = false;
        // Disable RX interrupts
        uart_rx_fifo_empty_set_mask(false);

        // Active? Read immediately.
        if (_rs485_readAvail() >= size) {
            ret = true;
            while (size > 0) {
                *(data++) = *(s_readPtr++);
                s_readPtr = s_readPtr % RS485_BUF_SIZE;
                size--;
                CLRWDT();
            }
        }

        // Re-enabled interrupts
        uart_rx_fifo_empty_set_mask(true);
        return ret;
    }   
}
