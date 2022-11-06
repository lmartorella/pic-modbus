#ifndef _FUSES_POSIX_H
#define _FUSES_POSIX_H

#undef DEBUGMODE

// Define IP and protocol
#define HAS_IP
#define RS485_BAUD 19200

#define RS485_BUF_SIZE 64

typedef uint32_t TICK_TYPE;
// Using gettime
#define TICKS_PER_SECOND (1000000u)
#define TICKS_PER_MILLISECOND (1000u)

#define EXC_STRING_T const char*
#define EXC_STRING_NULL ((void*)0)

void fatal(const char* str);

#define CLRWDT()

#endif /* FUSES_RASPBIAN_H */

