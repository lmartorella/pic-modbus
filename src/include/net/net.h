#ifndef _NET_H
#define	_NET_H

#include <xc.h>
#define HAS_EEPROM

#include <stdarg.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

void net_init(void);
_Bool net_poll(void);

/**
 * API
 */
#include "bus_client.h"
#include "leds.h"
#include "persistence.h"
#include "rs485.h"
#include "registers.h"
#include "timers.h"
#include "uart.h"

#endif	/* _NET_H */
