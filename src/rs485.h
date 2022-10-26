#ifndef RS485_H
#define	RS485_H

/**
 * High-level RS485 operation module virtualization
 */

/**
 * State of the wired line
 */
typedef enum {
    // Receiving, all OK
    RS485_LINE_RX = 1,
    // End of transmitting, in disengage line period
    RS485_LINE_TX_DISENGAGE = 2,
    // Transmitting, data
    RS485_LINE_TX = 3,
    // Wait before engaging the line
    RS485_LINE_WAIT_FOR_ENGAGE = 4,
    // After engaged, wait before transmitting
    RS485_LINE_WAIT_FOR_START_TRANSMIT = 5,
} RS485_STATE;

extern RS485_STATE rs485_state;

/**
 * Initialize asynchronous mode, but only half-duplex is used
 */
void rs485_init();

/**
 * To feed/receive channel
 */
void rs485_interrupt();

/**
 * Poll as much as possible (internal timered)
 */
void rs485_poll();

// Enqueue bytes to send. Use 9-bit address. Buffer is copied (max. 32 bytes)
// Warning: the address bit is used immediately and not enqueued to the buffer
void rs485_write(_Bool address, const uint8_t* data, uint8_t size);
// Send a special OVER token to the bus when the transmission ends (if there are 
// data in TX queue)
extern __bit rs485_over;
// When rs485_over is set, close will determine with char to send
extern __bit rs485_close;
// When set after a write operation, remain in TX state when data finishes until next write operation
extern __bit rs485_master;

// Read data, if available.
__bit rs485_read(uint8_t* data, uint8_t size);

/**
 * Get count of available bytes in the read buffer
 */
uint8_t rs485_readAvail();

/**
 * Get count of available bytes in the write buffer
 */
uint8_t rs485_writeAvail();

// Get the last bit9 received
extern __bit rs485_lastRc9;
// Get/set the skip flag. If set, rc9 = 0 bytes are skipped by receiver
extern __bit rs485_skipData;

// Don't change the line status, but simulate a TX time for disengage
void rs485_waitDisengageTime();

// See OSI model document for timings.
// TICKS_PER_SECOND = 3906 on PIC16 @4MHz
// TICKS_PER_SECOND = 24414 on PIC18 @25MHz
// BYTES_PER_SECONDS = (BAUD / 11 (9+1+1)) = 1744 (round down) = 0.57ms
#define BYTES_PER_SECONDS (uint32_t)((RS485_BAUD - 11) / 11)
// 2 ticks per byte, but let's do 3 (round up) for PIC16 @4MHz
// 14 ticks per byte (round up) on PIC18 @25MHz
#define TICKS_PER_BYTE (TICK_TYPE)((TICKS_PER_SECOND + BYTES_PER_SECONDS) / BYTES_PER_SECONDS)

typedef enum {
    // ASCII EndOfTransmission
    // Used to close the socket communication
    RS485_CCHAR_CLOSE = 0x04,
    // ASCII SynchronousIdle
    // Used to tell that the socket should stay opened
    RS485_CCHAR_IDLE = 0x16,
    // ASCII EndOfTransmissionBlock
    // Used to switch over to other party
    RS485_CCHAR_OVER = 0x17,
} RS485_CONTROL_CHAR;

#ifdef DEBUGMODE
#define set_rs485_over() rs485_over=1;io_printChDbg('>');
#else
#define set_rs485_over() rs485_over=1;
#endif

#endif	/* RS485_H */

