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

typedef struct {
    uint8_t functionCount;
    uint8_t resetReason;
    char errMsg[14];
} AUTOCONF_NODE_STATUS;
/**
 * Register 0x0: get sinks count, sink status (reset reason), and exception code.
 */
void autoconf_readNodeStatus();

/**
 * Register 0x1: get sinks IDs. The buffer is 4bytes * functionCount
 */
void autoconf_readSinkIds();

/**
 * Register 0x2: get/set current node GUID as 16 raw bytes
 */
void autoconf_readNodeGuid();
void autoconf_writeNodeGuid();

#ifdef __cplusplus
}
#endif

#endif
