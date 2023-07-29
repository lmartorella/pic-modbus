#ifndef _TEST_LT820_CONF_H
#define _TEST_LT820_CONF_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdbool.h>
#include <stdint.h>
#include <unistd.h>

typedef _Bool __bit;

/**
 * Radio module setup
 */
#define RADIO_BUF_SIZE (16)

/**
 * LT8920 module setup
 */

// from 0 to 127
#define LT8920_CHANNEL (30)

// Choose unique sync words for each over-the-air network.
// Similar to a MAC address.
#define LT8920_SYNC_WORD_0 (0x3f12)
#define LT8920_SYNC_WORD_1 (0x8631)
#define LT8920_SYNC_WORD_2 (0xf17e)
#define LT8920_SYNC_WORD_3 (0x930f)

#undef LT8920_MAX_TX_POWER

// If true, use the PACK_LENGTH_EN feature to transmit the packet size in advance
#define LT8920_FIRST_BYTE_AS_LENGTH

#define _debug_print_init_reg(a,b,c)    debug_print_init_reg(a, b, c)
extern void debug_print_init_reg(uint8_t reg, uint16_t init_val, uint16_t set_val);

#ifdef __cplusplus
}
#endif

#endif
