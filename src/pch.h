#ifndef PCH_H
#define	PCH_H

#ifdef __XC8
#include <xc.h>
#define HAS_EEPROM
#endif

#ifdef _CONF_POSIX
typedef unsigned char __bit;

#define __PACK
// For netserver use
#define __POSIX
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

/**
 * Loads the MCU headers and additional fuses
 */
#if defined(_CONF_MCU_CARD)
#include "../../samples/beans/hardware/fuses_mcu_card.h"
#define _IS_ETH_CARD

#elif defined(_CONF_GARDEN_BEAN)
#include "../../samples/garden/fuses_garden_bean.h"
#define _IS_PIC16F887_CARD

#elif defined(_CONF_MICRO_BEAN)
#include "../../samples/beans/hardware/fuses_micro_bean.h"
#define _IS_PIC16F1827_CARD

#elif defined(_CONF_POSIX)
#include "../../samples/netmaster/hardware/fuses_raspbian.h"
#define _IS_RASPI_CARD

#else
#error Missing configuration
#endif

#ifdef __XC8
// Size optimized
__bit memcmp8(void* p1, void* p2, uint8_t size);
// Internal core clock drives timer with 1:256 prescaler
#define TICKS_PER_SECOND		(TICK_TYPE)((TICK_CLOCK_BASE + (TICK_PRESCALER / 2ull)) / TICK_PRESCALER)	
#define TICKS_PER_MSECOND		(TICK_TYPE)(TICKS_PER_SECOND / 1000)
#endif

void enableInterrupts();
void hw_init();

#endif	/* PCH_H */

