#ifndef _FUSES_POSIX_H
#define _FUSES_POSIX_H

#undef DEBUGMODE

// Define IP and protocol
#define HAS_IP
#define HAS_RS485_BUS_PRIMARY

#define RS485_BUF_SIZE 64

typedef uint32_t TICK_TYPE;
// Using gettime
#define TICKS_PER_SECOND (1000000u)
#define TICKS_PER_MILLISECOND (1000u)

#define LAST_EXC_TYPE const char*

void fatal(const char* str);

#define CLRWDT()

#endif /* FUSES_RASPBIAN_H */

