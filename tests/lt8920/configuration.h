#ifndef _TEST_LT820_CONF_H
#define _TEST_LT820_CONF_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdbool.h>
#include <stdint.h>
#include <unistd.h>

typedef _Bool __bit;

#define LT8920_BUF_SIZE (16)
// from 0 to 127
#define LT8920_CHANNEL (30)
// Choose unique sync words for each over-the-air network.
// Similar to a MAC address.
#define LT8920_SYNC_WORD_0 (0x0000)
#define LT8920_SYNC_WORD_1 (0x0000)
#define LT8920_SYNC_WORD_2 (0x0000)
#define LT8920_SYNC_WORD_3 (0x0000)

#define _debug_print_init_reg(a,b,c)    debug_print_init_reg(a, b, c)
extern void debug_print_init_reg(uint8_t reg, uint16_t init_val, uint16_t set_val);

#ifdef __cplusplus
}
#endif

#endif
