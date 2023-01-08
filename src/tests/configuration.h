#ifndef _TEST_CONF_H
#define _TEST_CONF_H

#define TICK_TYPE uint32_t
#define TICKS_PER_SECOND (1000000)
#define RS485_BAUD (19200)
#define RS485_BUF_SIZE (32)

#ifdef __cplusplus
extern "C" {
#endif

void fatal(const char* msg);

#ifdef __cplusplus
}
#endif

#endif
