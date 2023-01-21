#ifndef _STD_CONF_H
#define _STD_CONF_H

#include <stdbool.h>
#include <stdint.h>
#include <errno.h>
#include <unistd.h>

typedef _Bool __bit;

#define __PACK
#define __POSIX
#define _GNU_SOURCE

#define RS485_BAUD 19200

#undef DEBUGMODE

// Define IP and protocol
#define HAS_IP

#define RS485_BUF_SIZE (64)
#define STREAM_BUFFER_SIZE (16)

typedef uint32_t TICK_TYPE;
// Using gettime
#define TICKS_PER_SECOND (1000000u)
#define TICKS_PER_MILLISECOND (1000u)

typedef const char* EXC_STRING_T;
#define EXC_STRING_NULL ((void*)0)

#define CLRWDT()

#ifdef __cplusplus
extern "C" {
#endif

void fatal(const char* str);

#ifdef __cplusplus
}
#endif

#endif
