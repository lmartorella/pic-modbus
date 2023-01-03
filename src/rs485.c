#include "net/net.h"

RS485_STATE rs485_state;

#define ADJUST_PTR(x) while (x >= (s_buffer + RS485_BUF_SIZE)) x-= RS485_BUF_SIZE

// Circular buffer
static uint8_t s_buffer[RS485_BUF_SIZE];
// Pointer of the writing head
static uint8_t* s_writePtr;
// Pointer of the reading head (if = write ptr, no bytes avail)
static uint8_t* s_readPtr;
#define _rs485_readAvail() ((uint8_t)(((uint8_t)(s_writePtr - s_readPtr)) % RS485_BUF_SIZE))
#define _rs485_writeAvail() ((uint8_t)(((uint8_t)(s_readPtr - s_writePtr - 1)) % RS485_BUF_SIZE))

/**
 * Set when the incoming data can be safely skipped and won't trigger read buffer overrun
 * if not processed
 */
__bit rs485_skipData;

static __bit s_lastTx;
static __bit s_oerr;

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
    s_oerr  = false;
    s_lastTx = false;
    s_writePtr = s_readPtr = s_buffer;
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
#define writeByte() \
    uart_write(*(s_readPtr++)); \
    uart_tx_fifo_empty_set_mask(true); \
    ADJUST_PTR(s_readPtr);

/**
 * Process a RS485 interrupt (FIFO empty)
 */
void rs485_interrupt() {
    // Empty TX buffer? Check for more data
    if (uart_tx_fifo_empty() && uart_tx_fifo_empty_get_mask()) {
        do {
            CLRWDT();
            if (_rs485_readAvail() > 0) {
                // Feed more data, read at read pointer and then increase
                writeByte();
            } else {
                // NO MORE data to transmit
                // TX2IF cannot be cleared, shut IE
                uart_tx_fifo_empty_set_mask(false);
                // goto first phase of tx end
                s_lastTx = true;
                break;
            }
        } while (uart_tx_fifo_empty());
    } else if (!uart_rx_fifo_empty() && uart_rx_fifo_empty_get_mask()) {
        // Data received
        do {
            CLRWDT();
            
            uint8_t data;
            UART_RX_MD md;

            uart_read(&data, &md);
            
            if (md.oerr) {
                // Disable IE otherwise the interrupt will loop
                s_oerr = 1;
                uart_rx_fifo_empty_set_mask(false);
                return;
            }
            
            // if s_ferr don't disengage read, only set the flag, in order to not lose next bytes
            // Only read data (0) if enabled
            if (!md.ferr && (!rs485_skipData)) {
                rs485_lastRc9 = md.rc9;
                *(s_writePtr++) = data;
                ADJUST_PTR(s_writePtr);
            }
        } while (!uart_rx_fifo_empty());
    }
}

/**
 * Poll as much as possible
 */
_Bool rs485_poll() {
    CLRWDT();
    if (s_oerr) {
        fatal("U.OER");
        return false;
    }
    
    TICK_TYPE elapsed = timers_get() - s_lastTick;
    switch (rs485_state){
        case RS485_LINE_TX:
            if (s_lastTx) {
                s_lastTick = timers_get();
                rs485_state = RS485_LINE_TX_DISENGAGE;
                s_lastTx = false;
            }
            break;
        case RS485_LINE_WAIT_FOR_ENGAGE:
            if (elapsed >= ENGAGE_CHANNEL_TIMEOUT) {
                // Engage
                rs485_state = RS485_LINE_WAIT_FOR_START_TRANSMIT;
                // Enable RS485 driver
                uart_transmit();
                s_lastTick = timers_get();
            }
            break;
        case RS485_LINE_WAIT_FOR_START_TRANSMIT:
            if (elapsed >= START_TRANSMIT_TIMEOUT) {
                // Transmit
                rs485_state = RS485_LINE_TX;
                s_lastTx = false;
                if (_rs485_readAvail() > 0) {
                    // Feed first byte
                    writeByte();
                } else {
                    // Enable interrupts now to eventually change state
                    uart_tx_fifo_empty_set_mask(true);
                }
            }
            break;
        case RS485_LINE_TX_DISENGAGE:
            if (elapsed >= DISENGAGE_CHANNEL_TIMEOUT) {
                // Detach TX line
                rs485_state = RS485_LINE_RX;
                rs485_startRead();
            }
            break;
    }
    return rs485_state != RS485_LINE_RX;
}

void rs485_write(const uint8_t* data, uint8_t size) { 
    rs485_master = 0;

    // Reset reader, if in progress
    if (rs485_state == RS485_LINE_RX) {
        // Truncate reading
        uart_disable_rx();
        uart_rx_fifo_empty_set_mask(0);
        s_readPtr = s_writePtr = s_buffer;

        // Enable UART transmit. This will trigger the TXIF, but don't enable it now.
        uart_enable_tx();

        rs485_state = RS485_LINE_WAIT_FOR_ENGAGE;
        s_lastTick = timers_get();
    }

    // Disable interrupts
    uart_tx_fifo_empty_set_mask(false);

    if (size > _rs485_writeAvail()) {
        // Overflow error
        fatal("U.ov");
    }
    
    // Copy to buffer
    while (size > 0) {
        *(s_writePtr++) = *(data++);
        ADJUST_PTR(s_writePtr);
        size--;
        CLRWDT();
    }

    // Schedule trasmitting, if not yet ready
    switch (rs485_state) {
        case RS485_LINE_TX_DISENGAGE:
            // Re-convert it to tx
            rs485_state = RS485_LINE_WAIT_FOR_START_TRANSMIT;
            s_lastTick = timers_get();
            break;
        case RS485_LINE_TX:
            // Was in transmit state: reenable TX feed interrupt
            uart_tx_fifo_empty_set_mask(true);
            break;
    }
}

static void rs485_startRead() {
    CLRWDT();
    if (rs485_state != RS485_LINE_RX) {
        // Break all
        rs485_state = RS485_LINE_TX_DISENGAGE;
        s_lastTick = timers_get();
        return;
    }
    
    // Disable writing (and reset OERR)
    uart_disable_tx();
    uart_tx_fifo_empty_set_mask(false);

    // Disable RS485 driver
    uart_receive();
    rs485_state = RS485_LINE_RX;

    // Reset circular buffer
    s_readPtr = s_writePtr = s_buffer;

    // Enable UART receiver
    uart_enable_rx();
    uart_rx_fifo_empty_set_mask(1);
}

void rs485_waitDisengageTime() {
    if (rs485_state == RS485_LINE_RX) { 
        // Disable rx receiver
        uart_disable_rx();
        uart_rx_fifo_empty_set_mask(0);
        
        rs485_state = RS485_LINE_TX_DISENGAGE;
        s_lastTick = timers_get();
    }
}

__bit rs485_read(uint8_t* data, uint8_t size) {
    if (rs485_state != RS485_LINE_RX) { 
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
                ADJUST_PTR(s_readPtr);
                size--;
                CLRWDT();
            }
        }

        // Re-enabled interrupts
        uart_rx_fifo_empty_set_mask(true);
        return ret;
    }   
}
