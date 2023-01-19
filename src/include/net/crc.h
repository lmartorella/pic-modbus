#ifndef _CRC16_H
#define _CRC16_H

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * Current CRC, that is kept updated with all the bytes read/written.
 */
extern uint16_t crc16;

void crc_reset();
void crc_update(uint8_t ch);

#ifdef __cplusplus
}
#endif

#endif