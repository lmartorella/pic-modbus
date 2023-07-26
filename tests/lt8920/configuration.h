#ifndef _TEST_LT820_CONF_H
#define _TEST_LT820_CONF_H

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

#endif
