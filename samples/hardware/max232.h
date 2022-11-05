#ifndef XC_RS232_H
#define	XC_RS232_H

#ifdef HAS_MAX232_SOFTWARE

// Connection 9600,N,8,1
void max232_init();

extern uint8_t max232_buffer1[MAX232_BUFSIZE1];
extern uint8_t max232_buffer2[MAX232_BUFSIZE2];

// Disable interrupts. Send txSize byte from the buffer and then receive in the buffer, and returns the size
// Timeout of 0.05s of no data
signed char max232_sendReceive(int8_t txSize);

// Disable interrupts. Send txSize byte from the buffer
void max232_send(signed char txSize);

#endif
#endif	/* XC_RS232_H */

