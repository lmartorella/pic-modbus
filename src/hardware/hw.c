#include "../pch.h"

#ifdef _IS_ETH_CARD
#include "fuses_eth_card.inc"
#elif defined(_IS_PIC16F628_CARD)
#include "fuses_pic16f628.inc"
#elif defined(_IS_PIC16F1827_CARD)
#include "fuses_pic16f1827.inc"
#elif defined(_IS_PIC16F887_CARD)
#include "fuses_pic16f887.inc"
#else
#error Missing configuration
#endif

