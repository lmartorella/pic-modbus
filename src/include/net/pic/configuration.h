#ifndef _STD_CONF_H
#define _STD_CONF_H

#define RS485_BAUD 19200
#define le16toh(b) (b)
#define htole16(b) (b)

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

#else
#error Missing configuration
#endif

void enableInterrupts();

#endif
