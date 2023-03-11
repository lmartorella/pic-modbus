#ifndef SINKS_H_
#define SINKS_H_

/**
 * Definition for ModBus functions about the reflection API
 */

#include "configuration.h"

#ifdef __cplusplus
extern "C" {
#endif

void regs_init();

/**
 * Holding rgisters [0-2]
 */
typedef struct {
    /**
     * Set a new station node (RS485)
     */
    uint8_t stationNode;
    uint8_t _filler;

    /**
     * See RESET_REASON
     */
    uint8_t resetReason;
    uint8_t _filler2;
    
    /**
     * Count of CRC errors in the reading period
     */
    uint16_t crcErrors;
} SYS_REGISTERS;

#define REGS_COUNT (sizeof(SYS_REGISTERS) / 2)

extern __persistent SYS_REGISTERS regs_registers;

/**
 * Called when the registers are read
 */
void regs_onRead();

/**
 * Called when the registers are written
 */
void regs_onWrite();

#ifdef __cplusplus
}
#endif

#endif
