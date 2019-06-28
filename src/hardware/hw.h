#ifndef HW_H
#define	HW_H

/**
 * Loads the MCU headers and additional fuses
 */

#if defined(_CONF_TEST_ETH_CARD)
#include "../../../samples/beans/hardware/fuses_test_eth_card.h"
#define _IS_ETH_CARD

#elif defined(_CONF_MCU_CARD)
#include "../../../samples/beans/hardware/fuses_mcu_card.h"
#define _IS_ETH_CARD

#elif defined(_CONF_TEST_MINI_BEAN)
#include "../../../samples/beans/hardware/fuses_test_mini_bean.h"
#define _IS_PIC16F628_CARD

#elif defined(_CONF_SOLAR_BEAN)
#include "../../../samples/beans/hardware/fuses_solar_bean.h"
#define _IS_PIC16F628_CARD

#elif defined(_CONF_GARDEN_BEAN)
#include "../../../samples/beans/hardware/fuses_garden_bean.h"
#define _IS_PIC16F887_CARD

#elif defined(_CONF_MICRO_BEAN)
#include "../../../samples/beans/hardware/fuses_micro_bean.h"
#define _IS_PIC16F1827_CARD

#elif defined(_CONF_RASPBIAN)
#include "../../../samples/beans/hardware/fuses_raspbian.h"
#define _IS_RASPI_CARD

#else
#error Missing configuration
#endif

void enableInterrupts();
void hw_init();

#include "./eeprom.h"
#include "./leds.h"
#include "./uart.h"

#endif	/* HW_H */

