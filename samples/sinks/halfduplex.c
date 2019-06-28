#include "../../../src/nodes/pch.h"
#include "../../../src/nodes/protocol.h"
#include "halfduplex.h"

#if (defined(HAS_MAX232_SOFTWARE) || defined(HAS_FAKE_RS232)) && defined(HAS_BUS)

static struct {
    // 0xff -> echo data
    // 0xfe -> don't read any data
    BYTE mode;
    union {
        struct {
            // Size 16-bit, but only 7 are used (+sign)
            signed char countl;
            signed char counth; 
        };
        struct { 
            signed int count;
        };
    };
} s_header;
static signed char s_pos;
static BYTE* s_ptr;

static enum {
    ST_IDLE,
    // Receiving data, len OK
    ST_RECEIVE_SIZE,
    // Receiving data
    ST_RECEIVE_DATA,

    // Transmit data
    ST_TRANSMIT_DATA
} s_state;

void halfduplex_init()
{
    s_state = ST_IDLE;
    s_header.count = 0;
}

bit halfduplex_read()
{
    if (s_state != ST_RECEIVE_DATA) {
        // IN HEADER READ
        s_state = ST_RECEIVE_SIZE;
        // Wait for size first
        if (prot_control_readAvail() < sizeof(s_header)) {
            // Go on
            return 1;
        }
        // Have size
        prot_control_read(&s_header, sizeof(s_header));
        // Wait for data
        s_state = ST_RECEIVE_DATA;
        s_ptr = max232_buffer1;
        s_pos = 0;
    }
    
    // I'm in ST_RECEIVE_DATA mode
    while (prot_control_readAvail() && s_pos < s_header.countl) {
        prot_control_read(s_ptr, 1);
        s_pos++;
        // Read buffer data
        if (s_pos == MAX232_BUFSIZE1) {
            s_ptr = max232_buffer2;
        }
        else {
            s_ptr++;
        }
    }

    // Ask for more data?
    if (s_pos < s_header.countl) {
        // Again
        return 1;
    }
    else {
        // Else data OK
        s_state = ST_IDLE;
#ifdef DEBUGMODE
        io_printChDbg('@');
#endif
        // Stop data
        return 0;
    }
}

bit halfduplex_write()
{
    if (s_state != ST_TRANSMIT_DATA) {
        if (prot_control_writeAvail() < 2) {
            // Wait for buffer to be free first
            return 1;
        }
        
        s_header.counth = 0;
        switch (s_header.mode) {
            case 0xfe:
                // Don't read data, returns 0
                max232_send(s_header.countl);
                s_header.countl = 0;
                break;
            case 0xff:
                // Echoes data without using the UART line
                break;
            default:
                // Disable bus. Start read. Blocker.
                s_header.count = max232_sendReceive(s_header.countl);
                break;
        }
        
        // IN HEADER WRITE
        prot_control_writeW(s_header.count);
        // In case of error don't send any data back
        if (s_header.countl < 0) {
            s_header.count = 0;
        }
        s_ptr = max232_buffer1;
        s_pos = 0;
        s_state = ST_TRANSMIT_DATA;
    }

    // Write max 0x10 bytes at a time
    while (prot_control_writeAvail() && s_pos < s_header.countl) {
        prot_control_write(s_ptr, 1);
        s_pos++;
        if (s_pos == MAX232_BUFSIZE1) {
            s_ptr = max232_buffer2;
        }
        else {
            s_ptr++;
        }
    }
   
    // Ask for more data?
    if (s_pos < s_header.countl) {
        // Again
        return 1;
    }
    else { 
        // End of transmit task -> reset sink state
        s_state = ST_IDLE;
        s_header.count = 0;
        return 0;
    }
}

#endif