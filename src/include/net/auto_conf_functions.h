#ifndef SINKS_H_
#define SINKS_H_

/**
 * Definition for ModBus functions about the reflection API
 */

#include "configuration.h"
#include "net/guid.h"

#ifdef __cplusplus
extern "C" {
#endif

void autoconf_init();

/**
 * Used by function 0 when reading
 */
typedef struct {
    /**
     * Application functions, count
     */
    uint16_t functionCount;

    /**
     * Application functions, first slot (in 8 registers ranges)
     */
    uint16_t functionStart;

    /**
     * See RESET_REASON
     */
    uint16_t resetReason;
    
    /**
     * Count of CRC errors in the reading period
     */
    uint16_t crcErrors;
} AUTOCONF_READ_NODE_STATUS;

/**
 * Used by function 0 when writing
 */
typedef struct {
    /**
     * Reset error counters
     */
    uint16_t resetCounters;

    /**
     * If set, reset the node
     */
    uint16_t reset;

    /**
     * Set a new station node (RS485)
     */
    uint16_t stationNode;
} AUTOCONF_WRITE_NODE_STATUS;

/**
 * Register 0x0: get sinks count, sink status (reset reason), and exception code.
 * Write node address, reset counters.
 */
void autoconf_readNodeStatus();
void autoconf_writeNodeStatus();

/**
 * Register 0x1: get application function IDs. At every read, read 4 IDs (each ID is a fourcc, so 2 16-bit registers)
 * Continue to read to read the whole `functionCount`. 
 * Reading or writing the node status (address 0) resets the reading pointer to the begin.
 */
void autoconf_readSinkIds();

/**
 * Register 0x2: get/set current node GUID as 16 raw bytes
 */
void autoconf_readNodeGuid();
void autoconf_writeNodeGuid();

/**
 * The applicative function count in the `autoconf_appFunctionIds`. 
 */
extern const uint8_t autoconf_appFunctionCount;

/**
 * The unique ID of the named function, his type
 */
extern const FOURCC autoconf_appFunctionIds[];

#ifdef __cplusplus
}
#endif

#endif
