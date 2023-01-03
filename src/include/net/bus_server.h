#ifndef _MODBUS_SERVER_H
#define	_MODBUS_SERVER_H

/**
 * Common interface for Modbus server (primary) nodes.
 * It can be implemented, for instance, by RTU nodes (RS485 master), TCP servers or 
 * wireless radio server stations.
 *
 * Routers (e.g. from TCP to RTU) will run both client and server instances.
 */

/**
 * Initialize the server node
 */
void bus_srv_init();

/**
 * Poll for server activities.
 * Returns true if the node is currently active and requires short polling.
 * Returns false if the node is not currently active and it can be polled with larger period,
 * depending on the medium implementation (e.g. 1ms)
 */
_Bool bus_srv_poll(void);

/**
 * Stats about errors
 */
typedef struct {
    // Count of socket timeouts
    uint8_t socketTimeouts;
} BUS_SRV_STATS;
extern BUS_SRV_STATS bus_srv_stats;

/**
 * Get the server state machine current state
 */
typedef enum {
    BUS_SRV_STATE_NONE,
    BUS_SRV_STATE_SOCKET_CONNECTED,
    BUS_SRV_STATE_SOCKET_TIMEOUT,
    BUS_SRV_STATE_SOCKET_FRAME_ERR,
} BUS_SRV_STATE;
BUS_SRV_STATE bus_srv_getState(void);

/**
 * For auto-discovery of child stations
 */
extern __bit bus_srv_hasDirtyChildren;
extern const uint8_t bus_srv_childrenMaskSize;
// Bitset of bus_srv_getChildrenMaskSize size
extern uint8_t bus_srv_dirtyChildren[];
// Bitset of bus_srv_getChildrenMaskSize size
extern uint8_t bus_srv_knownChildren[];
void bus_srv_resetDirtyChildren(void);

#endif	/* BUS_H */

