#ifndef _HW_GPIO_H
#define	_HW_GPIO_H

#include "configuration.h"

#ifdef __cplusplus
extern "C" {
#endif

void gpio_init();

/**
 * Assert the reset pin for the required time
 */
void gpio_reset(_Bool asserted);

#ifdef __cplusplus
}
#endif

#endif	/* RS485_H */
