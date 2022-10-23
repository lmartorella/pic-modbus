#include "../../../src/nodes/pch.h"
#include "../../../src/nodes/bus_secondary.h"
#include "max232.h"

#if defined(HAS_MAX232_SOFTWARE) || defined(HAS_FAKE_RS232)

// Debug when RS232 RX line is sampled
#undef TIMING_DEBUG

BYTE max232_buffer1[MAX232_BUFSIZE1];
BYTE max232_buffer2[MAX232_BUFSIZE2];

void max232_init() {
#ifdef HAS_MAX232_SOFTWARE
    // Init I and O bits
    RS232_RX_TRIS = 1;
    RS232_TX_TRIS = 0;
    
    RS232_TX_PORT = 1; // unasserted
    
    RS232_TCON = RS232_TCON_OFF;
    RS232_TCON_REG = RS232_TCON_VALUE; 
#endif
}

#ifdef HAS_MAX232_SOFTWARE
#define RESETTIMER(t) { RS232_TCON_REG = t; RS232_TCON_ACC = 0; RS232_TCON_IF = 0; }
#define WAITBIT() { while(!RS232_TCON_IF) { CLRWDT(); } RS232_TCON_IF = 0; }
// Timeout to wait to receive the first byte: 0.5 seconds
#define TIMEOUT_FIRST (int)(RS232_BAUD * 0.5)
// Timeout after the last received byte
#define TIMEOUT_LAST (int)(RS232_BAUD * 0.01) // 96 bits -> ~10 bytes @9600

#define FRAME_ERROR -2
#define OVERFLOW_ERROR -1

static void send(BYTE b) {
    // Write a idle bit
    WAITBIT()
    // Write a START bit
    RS232_TX_PORT = 0;
    for (BYTE j = 0; j < 8; j++) {
        // Cycle bits
        WAITBIT()
        RS232_TX_PORT = b & 0x1;
        b >>= 1;
    }
    // Write a STOP bit
    WAITBIT()
    RS232_TX_PORT = 1;
}
#endif

void max232_send(signed char size) {
    if (size > MAX232_BUFSIZE1 + MAX232_BUFSIZE2) {
        fatal("UAs.ov");
    }

#ifdef HAS_MAX232_SOFTWARE
    INTCONbits.GIE = 0;

    RESETTIMER(RS232_TCON_VALUE)
    RS232_TCON = RS232_TCON_ON;
    WAITBIT()
    
    BYTE* ptr = max232_buffer1;
    for (signed char i = 0; i < size; i++) {
        send(*ptr);
        if (i == MAX232_BUFSIZE1 - 1) {
            ptr = max232_buffer2;
        }
        else {
            ptr++;
        }
        WAITBIT()
    }
    
    INTCONbits.GIE = 1;
#endif
}

// Write sync, disable interrupts
signed char max232_sendReceive(signed char size) {

    max232_send(size);

#ifdef HAS_MAX232_SOFTWARE
    INTCONbits.GIE = 0;

    // Now receive
    BYTE* ptr = max232_buffer1;
    signed char i = 0;

    // Set the timeout of the first byte
    int timeoutCount = TIMEOUT_FIRST;
    while (1) {
        // Wait for start bit (RX low)
        while (RS232_RX_PORT) {
            CLRWDT();
            if (RS232_TCON_IF) {
                RS232_TCON_IF = 0;
                if ((timeoutCount--) <= 0) {
                    INTCONbits.GIE = 1;
                    return i;
                }
            }
        }

        // Read bits
        // Sample in the middle
        RESETTIMER(RS232_TCON_VALUE_HALF)
        WAITBIT()
        RESETTIMER(RS232_TCON_VALUE)

        BYTE b = 0;
        for (char j = 0; j < 8; j++) {
            WAITBIT()
            b >>= 1;

            if (RS232_RX_PORT) {
                b = b | 0x80;
            }
            
#ifdef TIMING_DEBUG
            led_on();
            __delay_us(10); // 10% of the wave
            led_off();
#endif
        }

        // Wait for the stop bit
        WAITBIT();
        i++;
        if (!RS232_RX_PORT) {
            // Invalid STOP bit, Frame Error
            INTCONbits.GIE = 1;
            return FRAME_ERROR;
        }
        // Now in STOP state

        *ptr = b;
        CLRWDT();
        
        if (i == MAX232_BUFSIZE1) {
            ptr = max232_buffer2;
        }
        else if (i == MAX232_BUFSIZE1 + MAX232_BUFSIZE2) {
            // Buffer overflow
            INTCONbits.GIE = 1;
            return OVERFLOW_ERROR;
        }
        else {
            ptr++;
        }
        
        // Now change the timeout: use the shorter one now that at least 1 byte is received
        CLRWDT();
        timeoutCount = TIMEOUT_LAST;
    }   
#endif
}
        
#endif
