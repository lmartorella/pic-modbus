#include "pch.h"
#include "rs485.h"
#include "appio.h"

#ifdef HAS_RS485

#undef ETH_DEBUG_LINES

RS485_STATE rs485_state;

#define ADJUST_PTR(x) while (x >= (s_buffer + RS485_BUF_SIZE)) x-= RS485_BUF_SIZE

static BYTE s_buffer[RS485_BUF_SIZE];
// Pointer of the writing head
static BYTE* s_writePtr;
// Pointer of the reading head (if = write ptr, no bytes avail)
static BYTE* s_readPtr;

// Status of address bit in the serie
bit rs485_lastRc9;
bit rs485_skipData;
// Send a special OVER token to the bus when the transmission ends (if there are 
// data in TX queue)
bit rs485_over;
// When rs485_over is set, close will determine with char to send
bit rs485_close;
// When set after a write operation, remain in TX state when data finishes until next write operation
bit rs485_master;

static bit s_lastTx;
static bit s_oerr;

static TICK_TYPE s_lastTick;

// time to wait before engaging the channel (after other station finished to transmit)
#define ENGAGE_CHANNEL_TIMEOUT (TICK_TYPE)(TICKS_PER_BYTE * 1)  
// additional time to wait after channel engaged to start transmit
// Consider that the glitch produced engaging the channel can be observed as a FRAMEERR by other stations
// So use a long time here to avoid FERR to consume valid data
#define START_TRANSMIT_TIMEOUT (TICK_TYPE)(TICKS_PER_BYTE * 3)
// time to wait before releasing the channel = 2 bytes,
// but let's wait an additional byte since USART is free when still transmitting the last byte.
#define DISENGAGE_CHANNEL_TIMEOUT (TICK_TYPE)(TICKS_PER_BYTE * (2 + 1))

static void rs485_startRead();

void rs485_init()
{
    uart_init();
    uart_receive();
   
#ifdef ETH_DEBUG_LINES
    TRISDbits.RD0 = 0;
#endif
    
    rs485_skipData = 0;
    rs485_over = 0;
    rs485_close = 0;
    s_oerr  = 0;
    s_lastTx = 0;
    s_writePtr = s_readPtr = s_buffer;
    s_lastTick = TickGet();
    
    rs485_state = RS485_LINE_RX;
    rs485_startRead();
}

#define _rs485_readAvail() ((BYTE)(((BYTE)(s_writePtr - s_readPtr)) % RS485_BUF_SIZE))
#define _rs485_writeAvail() ((BYTE)(((BYTE)(s_readPtr - s_writePtr - 1)) % RS485_BUF_SIZE))

BYTE rs485_readAvail()
{
    if (rs485_state == RS485_LINE_RX) {
        return _rs485_readAvail();
    }
    else {
        return 0;
    }
}

BYTE rs485_writeAvail()
{
    if (rs485_state != RS485_LINE_RX) {
        return _rs485_writeAvail();
    }
    else {
        // Can switch to TX and have full buffer
        return RS485_BUF_SIZE - 1;
    }
}

// Feed more data, read at read pointer and then increase
// and re-enable interrupts now
#define writeByte() \
    uart_write(*(s_readPtr++)); \
    uart_tx_fifo_empty_set_mask(TRUE); \
    ADJUST_PTR(s_readPtr);

void rs485_interrupt()
{
    // Empty TX buffer? Check for more data
    if (uart_tx_fifo_empty() && uart_tx_fifo_empty_get_mask()) {
        do {
            CLRWDT();
            if (_rs485_readAvail() > 0) {
                // Feed more data, read at read pointer and then increase
                writeByte();
            }
            else if (rs485_master) {
                // Disable interrupt but remain in write mode
                uart_tx_fifo_empty_set_mask(FALSE);
                break;
            }
            else if (rs485_over) {
                rs485_over = 0;
                // Send OVER byte
                uart_set_9b(1);
                uart_write(rs485_close ? RS485_CCHAR_CLOSE : RS485_CCHAR_OVER);
                uart_tx_fifo_empty_set_mask(TRUE);
            } 
            else {
                // NO MORE data to transmit
                // TX2IF cannot be cleared, shut IE
                uart_tx_fifo_empty_set_mask(FALSE);
                // goto first phase of tx end
                s_lastTx = 1;
#ifdef ETH_DEBUG_LINES
                PORTDbits.RD0 = 1;
#endif
                break;
            }
        } while (uart_tx_fifo_empty());
    }
    else if (!uart_rx_fifo_empty() && uart_rx_fifo_empty_get_mask()) {
        // Data received
        do {
            CLRWDT();
            
            BYTE data;
            UART_RX_MD md;

            uart_read(&data, &md);
            
            if (md.oerr) {
                // Disable IE otherwise the interrupt will loop
                s_oerr = 1;
                uart_rx_fifo_empty_set_mask(FALSE);
                return;
            }
            
            // if s_ferr don't disengage read, only set the flag, in order to not lose next bytes
            // Only read data (0) if enabled
            if (!md.ferr && (md.rc9 || !rs485_skipData)) {
                rs485_lastRc9 = md.rc9;
                *(s_writePtr++) = data;
                ADJUST_PTR(s_writePtr);
            }
        } while (!uart_rx_fifo_empty());
    }
}

/**
 * Poll as much as possible (internal timered)
 */
void rs485_poll()
{
    CLRWDT();
    if (s_oerr) {
        fatal("U.OER");
    }
    
    TICK_TYPE elapsed = TickGet() - s_lastTick;
    switch (rs485_state){
        case RS485_LINE_TX:
            if (s_lastTx) {
                s_lastTick = TickGet();
                rs485_state = RS485_LINE_TX_DISENGAGE;
                s_lastTx = 0;
            }
            break;
        case RS485_LINE_WAIT_FOR_ENGAGE:
            if (elapsed >= ENGAGE_CHANNEL_TIMEOUT) {
                // Engage
                rs485_state = RS485_LINE_WAIT_FOR_START_TRANSMIT;
                // Enable RS485 driver
                uart_trasmit();
                s_lastTick = TickGet();
            }
            break;
        case RS485_LINE_WAIT_FOR_START_TRANSMIT:
            if (elapsed >= START_TRANSMIT_TIMEOUT) {
                // Transmit
                rs485_state = RS485_LINE_TX;
                s_lastTx = 0;
                if (_rs485_readAvail() > 0) {
                    // Feed first byte
                    writeByte();
                } else {
                    // Enable interrupts now to eventually change state
                    uart_tx_fifo_empty_set_mask(TRUE);
                }
            }
            break;
        case RS485_LINE_TX_DISENGAGE:
            if (elapsed >= DISENGAGE_CHANNEL_TIMEOUT) {
#ifdef ETH_DEBUG_LINES
                PORTDbits.RD0 = 0;
#endif
                // Detach TX line
                rs485_state = RS485_LINE_RX;
                rs485_startRead();
            }
            break;
    }
}

void rs485_write(BOOL address, const BYTE* data, BYTE size)
{ 
    rs485_over = rs485_close = rs485_master = 0;

    // Reset reader, if in progress
    if (rs485_state == RS485_LINE_RX) {
        // Truncate reading
        uart_disable_rx();
        uart_rx_fifo_empty_set_mask(0);
        s_readPtr = s_writePtr = s_buffer;

        // Enable UART transmit. This will trigger the TXIF, but don't enable it now.
        uart_enable_tx();

        rs485_state = RS485_LINE_WAIT_FOR_ENGAGE;
        s_lastTick = TickGet();
    }

    // Disable interrupts
    uart_tx_fifo_empty_set_mask(FALSE);

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

    // 9-bit address
    uart_set_9b(address);

    // Schedule trasmitting, if not yet ready
    switch (rs485_state) {
        case RS485_LINE_TX_DISENGAGE:
            // Re-convert it to tx
            rs485_state = RS485_LINE_WAIT_FOR_START_TRANSMIT;
            s_lastTick = TickGet();
            break;
        case RS485_LINE_TX:
            // Was in transmit state: reenable TX feed interrupt
            uart_tx_fifo_empty_set_mask(TRUE);
            break;
            
        //case RS485_LINE_TX:
        //case RS485_LINE_WAIT_FOR_START_TRANSMIT:
        //case RS485_LINE_WAIT_FOR_ENGAGE:
            // Already tx, ok
        //    break;
    }
}

static void rs485_startRead()
{
    CLRWDT();
    if (rs485_state != RS485_LINE_RX) {
        // Break all
        rs485_state = RS485_LINE_TX_DISENGAGE;
        s_lastTick = TickGet();
        return;
    }
    
    // Disable writing (and reset OERR)
    uart_disable_tx();
    uart_tx_fifo_empty_set_mask(FALSE);

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
        s_lastTick = TickGet();
    }
}

bit rs485_read(BYTE* data, BYTE size)
{
    if (rs485_state != RS485_LINE_RX) { 
        rs485_startRead();
        return FALSE;
    }
    else {
        static bit ret = FALSE;
        // Disable RX interrupts
        uart_rx_fifo_empty_set_mask(FALSE);

        // Active? Read immediately.
        if (_rs485_readAvail() >= size) {
            ret = TRUE;
            while (size > 0) {
                *(data++) = *(s_readPtr++);
                ADJUST_PTR(s_readPtr);
                size--;
                CLRWDT();
            }
        }

        // Re-enabled interrupts
        uart_rx_fifo_empty_set_mask(TRUE);
        return ret;
    }   
}

#endif
