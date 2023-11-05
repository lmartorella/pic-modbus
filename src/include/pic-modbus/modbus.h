#ifndef _PIC_MODBUS_H
#define	_PIC_MODBUS_H

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
#include "bus_client.h"
#include "rs485.h"
#include "sys.h"
#include "timers.h"
#include "uart.h"

#endif	/* _PIC_MODBUS_H */
