#ifndef BUS_H
#define	BUS_H

/**
 * Wired bus communication module (both master on bean nodes)
 */

#ifdef HAS_BUS

#ifdef HAS_RS485
#ifdef HAS_IP
#define HAS_BUS_SERVER
#else
#define HAS_BUS_CLIENT
#endif
#endif // HAS_RS485

void bus_init();
// Poll general bus activities
void bus_poll();
bit bus_isIdle();

typedef enum { 
    // Message to beat a bean
    BUS_MSG_TYPE_HEARTBEAT = 1,
    // Message to ask unknown bean to present (broadcast)
    BUS_MSG_TYPE_READY_FOR_HELLO = 2,
    // Message to ask the only unknown bean to register itself
    BUS_MSG_TYPE_ADDRESS_ASSIGN = 3,
    // Command/data will follow: socket open
    BUS_MSG_TYPE_CONNECT = 4
} BUS_MSG_TYPE;

typedef enum { 
    // Bean: ack heartbeat
    BUS_ACK_TYPE_HEARTBEAT = 0x20,
    // Bean: notify unknown (response to BUS_MSG_TYPE_READY_FOR_HELLO)
    BUS_ACK_TYPE_HELLO = 0x21
} BUS_ACK_TYPE;

#define UNASSIGNED_SUB_ADDRESS 0xff

#ifdef HAS_BUS_SERVER

// 8*8 = 63 max children (last is broadcast)
#define BUFFER_MASK_SIZE ((MASTER_MAX_CHILDREN + 7) / 8)

typedef enum {
    BUS_STATE_NONE,
    BUS_STATE_SOCKET_CONNECTED,
    BUS_STATE_SOCKET_TIMEOUT,
    BUS_STATE_SOCKET_FERR,
} BUS_STATE;

typedef struct {
    // Count of socket timeouts
    char socketTimeouts;
} BUS_MASTER_STATS;
extern BUS_MASTER_STATS g_busStats;

// Select a child, and start a private communication bridging the IP protocol socket.
void bus_connectSocket(int nodeIdx);
void bus_disconnectSocket(int val);

// Is still in command execution, waiting for command data receive complete?
BUS_STATE bus_getState();

// Dirty children
extern bit bus_hasDirtyChildren;
extern BYTE bus_dirtyChildren[BUFFER_MASK_SIZE];
void bus_resetDirtyChildren();

// Get active children mask & size
int bus_getChildrenMaskSize();
const BYTE* bus_getChildrenMask();

// Low-level socket state. The byte can be used on the wire as break char
typedef enum {
    // Normal state when not connected
    SOCKET_NOT_CONNECTED = -2,
    // Socket data timeout (no data in BUS_SOCKET_TIMEOUT)
    SOCKET_ERR_TIMEOUT = -3,
    // When a frame error on the wire
    SOCKET_ERR_FERR = -4,
    // When the server (IP) closes the socket
    SOCKET_ERR_CLOSED_BY_PARENT = -5
} SOCKET_STATE;

#endif

#endif

#endif	/* BUS_H */

