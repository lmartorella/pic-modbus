#ifndef _MODBUS_CLIENT_H
#define	_MODBUS_CLIENT_H

#include "configuration.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * Common interface for Modbus server (secondary) nodes.
 * It can be implemented, for instance, by RTU nodes (RS485 client), TCP clients or 
 * wireless radio client stations.
 *
 * Routers (e.g. from TCP to RTU) will run both client and server instances.
 */

/**
 * Initialize the client node
 */
void bus_cl_init();

/**
 * Poll for bus client activities.
 * Returns true if the node is currently active and requires short polling.
 * Returns false if the node is not currently active and it can be polled with larger period,
 * depending on the medium implementation (e.g. 1ms)
 */
__bit bus_cl_poll();

/**
 * Get the client state machine current state
 */
typedef enum {
    BUS_CL_STATE_HEADER_0 = 0,         // header0, 55
    BUS_CL_STATE_HEADER_1 = 1,         // header1, aa
    BUS_CL_STATE_HEADER_2 = 2,         // header2, address
    BUS_CL_STATE_HEADER_ADDRESS = 2,   // header2
    BUS_CL_STATE_HEADER_3 = 3,         // msgtype
            
    BUS_CL_STATE_SOCKET_OPEN = 10,
    BUS_CL_STATE_WAIT_TX
} BUS_CL_STATE;
BUS_CL_STATE bus_cl_getState(void);

#ifdef __cplusplus
}
#endif

#endif	/* BUS_SEC_H */
