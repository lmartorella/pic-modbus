#ifndef HW_H
#define	HW_H

#if defined(_CONF_TEST_ETH_CARD)
#include "fuses_test_eth_card.h"
#define _IS_ETH_CARD

#elif defined(_CONF_MCU_CARD)
#include "fuses_mcu_card.h"
#define _IS_ETH_CARD

#elif defined(_CONF_TEST_MINI_BEAN)
#include "fuses_test_mini_bean.h"
#define _IS_PIC16F628_CARD

#elif defined(_CONF_SOLAR_BEAN)
#include "fuses_solar_bean.h"
#define _IS_PIC16F628_CARD

#elif defined(_CONF_GARDEN_BEAN)
#include "fuses_garden_bean.h"
#define _IS_PIC16F887_CARD

#elif defined(_CONF_MICRO_BEAN)
#include "fuses_micro_bean.h"
#define _IS_PIC16F1827_CARD

#elif defined(_CONF_RASPBIAN)
#include "fuses_raspbian.h"
#define _IS_RASPI_CARD

#else
#error Missing configuration
#endif

void enableInterrupts();

#include "./cm1602.h"
#include "./eeprom.h"
#include "./i2c.h"
#include "./leds.h"
#include "./max232.h"
#include "./spi.h"
#include "./uart.h"
#include "./vs1011e.h"

#endif	/* HW_H */

