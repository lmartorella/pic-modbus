#ifndef PCH_H
#define	PCH_H

#ifdef __XC8
#include <xc.h>
#define HAS_EEPROM
#endif

#ifdef _CONF_LINUX
typedef unsigned char __bit;

#define __PACK
// For netserver use
#define __GNU
// For POSIX extension
#define _GNU_SOURCE

#include <errno.h>
#include <unistd.h>

#endif

#include <stdarg.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#include "hardware/hw.h"
#include "hardware/wait.h"
#include "guid.h"

#ifdef __XC8
// Size optimized
__bit memcmp8(void* p1, void* p2, uint8_t size);
// Internal core clock drives timer with 1:256 prescaler
#define TICKS_PER_SECOND		(TICK_TYPE)((TICK_CLOCK_BASE + (TICK_PRESCALER / 2ull)) / TICK_PRESCALER)	
#define TICKS_PER_MSECOND		(TICK_TYPE)(TICKS_PER_SECOND / 1000)
#endif

#include "hardware/tick.h"

#ifdef HAS_RS485
#include "rs485.h"
#endif

#endif	/* PCH_H */

