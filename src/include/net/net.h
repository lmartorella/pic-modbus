#ifndef _NET_H
#define	_NET_H

#ifdef __XC8

#include <xc.h>
#define HAS_EEPROM

#elif defined _CONF_POSIX

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

void net_init(void);
_Bool net_poll(void);

/**
 * API
 */
#include "auto_conf_functions.h"
#include "bus_client.h"
#include "leds.h"
#include "persistence.h"
#include "rs485.h"
#include "sys.h"
#include "timers.h"
#include "uart.h"

#endif	/* _NET_H */
