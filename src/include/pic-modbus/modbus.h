#ifndef _PIC_MODBUS_H
#define	_PIC_MODBUS_H

#define HAS_EEPROM

#include <stdarg.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

void modbus_init(void);
_Bool modbus_poll(void);

/**
 * API
 */
#include "rtu_client.h"
#include "sys.h"
#include "timers.h"

#endif	/* _PIC_MODBUS_H */
