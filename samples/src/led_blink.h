#ifndef LED_BLINK_H
#define	LED_BLINK_H

#include "./sample_config.h"

#ifdef	__cplusplus
extern "C" {
#endif
    
void blinker_init();
void blinker_poll();
_Bool blinker_conf();

typedef struct {
    TICK_TYPE period;
} LedBlinkRegsiters;

extern LedBlinkRegsiters blinker_regs;

#define LEDBLINK_REGS_ADDRESS (512)
#define LEDBLINK_REGS_ADDRESS_BE (LE_TO_BE_16(LEDBLINK_REGS_ADDRESS))
#define LEDBLINK_REGS_COUNT (sizeof(LedBlinkRegsiters) / 2)

#ifdef	__cplusplus
}
#endif

#endif	/* LED_BLINK_H */

