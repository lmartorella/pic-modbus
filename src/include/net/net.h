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

#define RS485_BAUD 19200

/**
 * Loads the MCU headers and additional fuses
 */
#if defined(_CONF_ETH_CARD)
#include "fuses_eth_card.h"
#define _IS_ETH_CARD

#elif defined(_CONF_GARDEN_BEAN)
#include "fuses_garden_bean.h"
#define _IS_PIC16F887_CARD

#elif defined(_CONF_MICRO_BEAN)
#include "fuses_micro_bean.h"
#define _IS_PIC16F1827_CARD

#elif defined(_CONF_POSIX)
#include "fuses_posix.h"

#else
#error Missing configuration
#endif


/**
 * CONSTANTS
 */
#define MASTER_MAX_CHILDREN 16

void net_prim_init(uint16_t serverUdpPort);
void net_sec_init(void);
_Bool net_prim_poll(void);
_Bool net_sec_poll(void);

#if defined(HAS_RS485_BUS_PRIMARY)
#define net_init net_prim_init
#define net_poll net_prim_poll
#elif defined(HAS_RS485_BUS_SECONDARY)
#define net_init net_sec_init
#define net_poll net_sec_poll
#endif


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
#include "uart.h"
#include "leds.h"

#endif	/* _NET_H */
