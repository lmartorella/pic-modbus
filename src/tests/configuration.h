#ifndef _TEST_CONF_H
#define _TEST_CONF_H

#include <stdbool.h>
#include <stdint.h>

typedef _Bool __bit;
typedef uint32_t TICK_TYPE;
#define TICKS_PER_SECOND (1000000)
#define RS485_BAUD (19200)
#define RS485_BUF_SIZE (16)

typedef const char* EXC_STRING_T;

#ifdef __cplusplus
extern "C" {
#endif

void fatal(const char* msg);

#ifdef __cplusplus
}
#endif

#endif
