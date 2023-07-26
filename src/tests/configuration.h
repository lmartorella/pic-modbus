#ifndef _TEST_CONF_H
#define _TEST_CONF_H

#include <endian.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdlib.h>

#define _CONF_RS485

typedef _Bool __bit;
typedef uint32_t TICK_TYPE;
#define __persistent
typedef struct {
    _Bool OERR;
    _Bool FERR;
} UART_ERR_BITS;

#define TICKS_PER_SECOND (1000000)
#define RS485_BAUD (19200)
#define RS485_BUF_SIZE (16)
#define STATION_NODE (2)

typedef const char* EXC_STRING_T;

#ifdef __cplusplus
extern "C" {
#endif

void fatal(const char* msg);
void RESET();

#ifdef __cplusplus
}
#endif

#endif
