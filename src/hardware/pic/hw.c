#include "../../../../src/nodes/pch.h"

/**
 * Uses .inc file to include compile-time code
 */

#ifdef _IS_ETH_CARD
#include "fuses_eth_card.c"
#elif defined(_IS_PIC16F628_CARD)
#include "fuses_pic16f628.c"
#elif defined(_IS_PIC16F1827_CARD)
#include "fuses_pic16f1827.c"
#elif defined(_IS_PIC16F887_CARD)
#include "fuses_pic16f887.c"
#else
#error Missing configuration
#endif

