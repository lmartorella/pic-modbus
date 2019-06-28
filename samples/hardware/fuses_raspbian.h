#ifndef FUSES_RASPBIAN_H
#define FUSES_RASPBIAN_H

#undef DEBUGMODE

// Define IP and protocol
#define HAS_IP
#define HAS_BUS
#define HAS_RS485

#ifdef DEBUG
// 17008 is the debug port
#define SERVER_CONTROL_UDP_PORT 17008
#else
// 17007 is the release port
#define SERVER_CONTROL_UDP_PORT 17007
#endif

#define CLIENT_TCP_PORT 20000

#define RS485_BAUD 19200
#define RS485_BUF_SIZE 64

typedef DWORD TICK_TYPE;
// Using gettime
#define TICKS_PER_SECOND (1000000u)

void CLRWDT();
void enableInterrupts();
void rom_poll();
void fatal(const char* str);

#endif /* FUSES_RASPBIAN_H */

