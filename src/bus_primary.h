#ifndef BUS_PRIM_H
#define	BUS_PRIM_H

/**
 * Wired bus communication module, for net master
 */

void bus_prim_init();
// Poll general bus activities
void bus_prim_poll();
__bit bus_prim_isIdle();

// 8*8 = 63 max children (last is broadcast)
#define BUFFER_MASK_SIZE ((MASTER_MAX_CHILDREN + 7) / 8)

typedef struct {
    // Count of socket timeouts
    char socketTimeouts;
} BUS_PRIMARY_STATS;

extern BUS_PRIMARY_STATS bus_prim_busStats;

// Select a child, and start a private communication bridging the IP protocol socket.
void bus_prim_connectSocket(int nodeIdx);
void bus_prim_disconnectSocket(int val);

typedef enum {
    BUS_STATE_NONE,
    BUS_STATE_SOCKET_CONNECTED,
    BUS_STATE_SOCKET_TIMEOUT,
    BUS_STATE_SOCKET_FRAME_ERR,
} BUS_PRIMARY_STATE;

// Is still in command execution, waiting for command data receive complete?
BUS_PRIMARY_STATE bus_prim_getState();

// Dirty children
extern __bit bus_prim_hasDirtyChildren;
extern uint8_t bus_prim_dirtyChildren[BUFFER_MASK_SIZE];
extern uint8_t bus_prim_knownChildren[BUFFER_MASK_SIZE];
void bus_prim_resetDirtyChildren();

// Get active children mask & size
int bus_prim_getChildrenMaskSize();
const uint8_t* bus_prim_getChildrenMask();

#endif	/* BUS_H */

