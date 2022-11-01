#ifndef _NET_H
#define	_NET_H

#ifdef __XC8

#include <xc.h>
#define HAS_EEPROM

#elif defined _CONF_POSIX

typedef unsigned char __bit;

#define __PACK
#define __POSIX
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
#if defined(_CONF_ETH_CARD)
#include "../hardware/pic/fuses_eth_card.h"
#define _IS_ETH_CARD

#elif defined(_CONF_GARDEN_BEAN)
#include "../hardware/pic/fuses_garden_bean.h"
#define _IS_PIC16F887_CARD

#elif defined(_CONF_MICRO_BEAN)
#include "../hardware/pic/fuses_micro_bean.h"
#define _IS_PIC16F1827_CARD

#elif defined(_CONF_POSIX)
#include "../hardware/posix/fuses_posix.h"

#else
#error Missing configuration
#endif


/**
 * CONSTANTS
 */

#ifdef DEBUG
// 17008 is the debug port
#define SERVER_CONTROL_UDP_PORT 17008
#else
// 17007 is the release port
#define SERVER_CONTROL_UDP_PORT 17007
#endif

#define CLIENT_TCP_PORT 20000
#define MASTER_MAX_CHILDREN 16

#define RS485_BAUD 19200

/**
 * API
 */
#include "appio.h"
#include "persistence.h"
#include "protocol.h"
#include "sinks.h"
#include "bus_primary.h"
#include "bus_secondary.h"
#include "ip_client.h"
#include "rs485.h"
#include "timers.h"
#include "leds.h"

#endif	/* _NET_H */
