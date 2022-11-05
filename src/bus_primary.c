#include "net/net.h"

/**
 * Wired bus communication module for master nodes
 */

uint8_t bus_prim_knownChildren[BUFFER_MASK_SIZE];
uint8_t bus_prim_dirtyChildren[BUFFER_MASK_SIZE];
__bit bus_prim_hasDirtyChildren;

// Ack message contains ACK
#define ACK_MSG_SIZE 4
#define BROADCAST_ADDRESS 0xff

static uint8_t s_scanIndex;
static TICK_TYPE s_lastScanTime;
static TICK_TYPE s_lastTime;

// Socket connected to child, or SOCKET_STATE
static int8_t s_socketConnected;

#define BUS_SCAN_TIMEOUT (TICK_TYPE)(TICKS_PER_SECOND * 1.5) // 1500ms 
#define BUS_SOCKET_TIMEOUT (TICK_TYPE)(TICKS_PER_SECOND * 2)  // 2000ms, SOLAR bean needs some time to sync wait for RS232 data
// ack time is due to engage+disengage time (of the slave) + ack_size
#define BUS_ACK_TIMEOUT (TICK_TYPE)(TICKS_PER_BYTE * ACK_MSG_SIZE * 4) // 9.1ms (19200,9,1 4*3bytes = )

static enum {
    // To call next child
    BUS_PRIV_STATE_IDLE,
    // Wait for ack
    BUS_PRIV_STATE_WAIT_ACK,
    // A direct connection is established
    BUS_PRIV_STATE_SOCKET_CONNECTED
} s_busState;

BUS_PRIMARY_STATS bus_prim_busStats;

static __bit s_waitTxFlush;
static __bit s_waitTxQuickEnd;

static void socketCreate();
static void socketPoll();

static __bit isChildKnown(uint8_t i) {
    return (bus_prim_knownChildren[i / 8] & (1 << (i % 8))) != 0;
}

static uint8_t countChildren() {
    uint8_t count = 0;
    for (uint8_t i = 0; i < sizeof(bus_prim_knownChildren); i++) {
        uint8_t d = bus_prim_knownChildren[i];
        while(d) {
            count += (d & 1);
            d >>= 1;
        }
    }
    return count;
}

static void setDirtyChild(uint8_t i)
{
    flog("setDirty: @%u, knownTot: %u", (unsigned)i, (unsigned)countChildren());
    bus_prim_dirtyChildren[i / 8] |= (1 << (i % 8));
}

static void updateDisp() {
    char msg[16];
    sprintf(msg, "%u nodes", countChildren());
    io_println(msg);
}

static void setChildKnown(uint8_t i) {
    setDirtyChild(i);
    bus_prim_knownChildren[i / 8] |= (1 << (i % 8));
    flog("setKnown: @%u, knownTot: %u", (unsigned)i, (unsigned)countChildren());    
    updateDisp();
}

static void setChildDead(uint8_t i) {
    setDirtyChild(i);
    bus_prim_knownChildren[i / 8] &= ~(1 << (i % 8));
    flog("setDead: @%u, knownTot: %u", (unsigned)i, (unsigned)countChildren());
    updateDisp();
}

void bus_prim_init() {
    ip_prot_init();

    // No beans are known, nor dirty
    memset(bus_prim_knownChildren, 0, BUFFER_MASK_SIZE);
    bus_prim_resetDirtyChildren();
    
    // Starts from zero
    s_scanIndex = BROADCAST_ADDRESS;
    s_socketConnected = SOCKET_NOT_CONNECTED;
    s_waitTxFlush = 0;
    s_waitTxQuickEnd = 0;
    
    // Do full scan
    s_lastScanTime = timers_get();

    memset(&bus_prim_busStats, 0, sizeof(BUS_PRIMARY_STATS));
}

void bus_prim_resetDirtyChildren() {
    if (bus_prim_hasDirtyChildren) {
        flog("resetDirty");
    }
    bus_prim_hasDirtyChildren = 0;
    memset(bus_prim_dirtyChildren, 0, BUFFER_MASK_SIZE);
}

// Ask for the next known child
static void scanNext()
{
    BUS_MSG_TYPE msgType = BUS_MSG_TYPE_HEARTBEAT;
    s_lastScanTime = timers_get();
    
    // Poll next child 
    s_scanIndex++;
    if (s_scanIndex >= MASTER_MAX_CHILDREN) {
        // Do broadcast now
        s_scanIndex = BROADCAST_ADDRESS;
        msgType = BUS_MSG_TYPE_READY_FOR_HELLO;
    }
    
    // Poll the line. Send sync
    uint8_t buffer[4] = { 0x55, 0xaa };
    buffer[2] = s_scanIndex;
    buffer[3] = msgType;
    rs485_write(true, buffer, 4);

    // Wait for tx end and then for ack
    s_waitTxFlush = 1;
    s_busState = BUS_PRIV_STATE_WAIT_ACK;
}

static void registerNewNode() {
    flog("Registering new children");
    
    // Find a free slot
    s_scanIndex = 0;
    // Check for valid node
    while (isChildKnown(s_scanIndex)) {
        s_scanIndex++;
        if (s_scanIndex >= MASTER_MAX_CHILDREN) {
            // Ops, no space
            s_busState = BUS_PRIV_STATE_IDLE;
            s_scanIndex = 0;
            
            io_println("Children full");
            
            return;
        }
    };
    
    // Have the good address
    // Do not store it now, store it after the ack
    flog("Found index %d", s_scanIndex);

    // Send it
    uint8_t buffer[4] = { 0x55, 0xaa };
    buffer[2] = s_scanIndex;
    buffer[3] = BUS_MSG_TYPE_ADDRESS_ASSIGN;
    rs485_write(true, buffer, 4);

    // Go ahead. Expect a valid response like the heartbeat
    s_waitTxFlush = 1;
    s_busState = BUS_PRIV_STATE_WAIT_ACK;
}

static void checkAck()
{
    uint8_t buffer[ACK_MSG_SIZE];
    // Receive bytes
    // Expected: 0x55, 0xaa, [index], [state] with rc9 = 0 + EXC
    rs485_read(buffer, ACK_MSG_SIZE);
    if (!rs485_lastRc9 && buffer[0] == 0x55 && buffer[1] == 0xaa && buffer[2] == s_scanIndex) {
        // Ok, good response
        switch (buffer[3]) { 
        case BUS_ACK_TYPE_HELLO:
            if (s_scanIndex == BROADCAST_ADDRESS) {
                // Need registration.
                registerNewNode();
                return;
            }
            else if (isChildKnown(s_scanIndex)) {
                // A known node reset! Notify it.
                bus_prim_hasDirtyChildren = 1;
                setDirtyChild(s_scanIndex);
            }
            else {
                // Not known node hello: register it
                bus_prim_hasDirtyChildren = 1;
                setChildKnown(s_scanIndex);
            }
            break;
        case BUS_ACK_TYPE_HEARTBEAT:
            if (!isChildKnown(s_scanIndex) && s_scanIndex != BROADCAST_ADDRESS) {
                // A node with address registered, but I didn't knew it. Register it.
                bus_prim_hasDirtyChildren = 1;
                setChildKnown(s_scanIndex);
            }
            break;
        case BUS_ACK_TYPE_READ_STATUS:
            // Set as dirty node (status to fetch)
            if (isChildKnown(s_scanIndex)) {
                bus_prim_hasDirtyChildren = 1;
                setDirtyChild(s_scanIndex);
            }
            break;
        }
    }
    // Next one.
    s_busState = BUS_PRIV_STATE_IDLE;
}

_Bool bus_prim_poll() {   
    if (s_waitTxFlush) {
        // Finished TX?
        if (rs485_state != RS485_LINE_RX) {
            // Skip state management
            return true;
        } else {
            // Start timeout and go ahead
            s_lastTime = timers_get();
            s_waitTxFlush = 0;
        }
    } else if (s_waitTxQuickEnd) {
        // Finished TX?
        if (rs485_state >= RS485_LINE_TX) {
            // Skip state management
            return true;
        } else {
            // Start timeout and go ahead
            s_lastTime = timers_get();
            s_waitTxQuickEnd = 0;
        }
    }
 
    _Bool isActive = true;
    uint8_t s = rs485_readAvail();
    switch (s_busState) {
        case BUS_PRIV_STATE_IDLE:
            // Should open a socket?
            if (s_socketConnected >= 0) {
                socketCreate();
            } else {
                // Do/doing a scan?
                if (timers_get() - s_lastScanTime >= BUS_SCAN_TIMEOUT) {
                    scanNext();
                } else {
                    isActive = false;
                }
            }
            break;
        case BUS_PRIV_STATE_WAIT_ACK:
            // Wait timeout for response
            if (s >= ACK_MSG_SIZE) {
                // Check what is received
                checkAck();
            } else {
                // Check for timeout
                if (timers_get() - s_lastTime >= BUS_ACK_TIMEOUT) {
                    // Timeout. Dead bean?
                    if (s_scanIndex != BROADCAST_ADDRESS && isChildKnown(s_scanIndex)) {
                        setChildDead(s_scanIndex);
                    }
                    s_busState = BUS_PRIV_STATE_IDLE;
                }
            }
            break;
        case BUS_PRIV_STATE_SOCKET_CONNECTED:
            if (timers_get() - s_lastTime >= BUS_SOCKET_TIMEOUT) {
                // Timeout. Dead bean?
                // Drop the TCP connection and reset the channel
                bus_prim_disconnectSocket(SOCKET_ERR_TIMEOUT);
                prot_prim_control_abort();
            } else {
                socketPoll();
            }
            break;
    }
    return isActive;
}

// The command starts when the bus is idle
void bus_prim_connectSocket(int8_t nodeIdx)
{
    s_socketConnected = nodeIdx;
    // Next IDLE will start the connection
}

void bus_prim_disconnectSocket(int8_t val)
{
    if (s_socketConnected >= 0) {
        // Send break char
        rs485_write(true, (uint8_t*)(&val), 1);
        s_waitTxFlush = 1;
        
        s_busState = BUS_PRIV_STATE_IDLE;
        
        bus_prim_busStats.socketTimeouts++;
    }
    s_socketConnected = val;
}

BUS_PRIMARY_STATE bus_prim_getState() 
{
    if (s_socketConnected >= 0)
        return BUS_STATE_SOCKET_CONNECTED;
    if (s_socketConnected == SOCKET_ERR_TIMEOUT) 
        return BUS_STATE_SOCKET_TIMEOUT;
    if (s_socketConnected == SOCKET_ERR_FRAME_ERR) 
        return BUS_STATE_SOCKET_FRAME_ERR;
    return BUS_STATE_NONE;
}

static void socketCreate() 
{
    // Bus is idle. Start transmitting/receiving.
    uint8_t buffer[4] = { 0x55, 0xaa };
    buffer[2] = (uint8_t)s_socketConnected;
    buffer[3] = BUS_MSG_TYPE_CONNECT;
    rs485_write(true, buffer, 4);

    // Don't wait the TX channel to be free, but immediately enqueue socket data, to avoid engage/disengage time and glitches
    // However wait for TX9 to be reusable, so wait for TX to be finished
    s_waitTxQuickEnd = 1;
    // Next state (after TX finishes) is IN COMMAND
    s_busState = BUS_PRIV_STATE_SOCKET_CONNECTED;
}

static void socketPoll() {
    // Bus line is slow, though
    uint8_t buffer[RS485_BUF_SIZE / 2];
    _Bool over = 0, close = 0;
            
    // Data from IP?
    uint16_t rx = prot_prim_control_readAvail();
    if (rx > 0) {
        // Read data and push it into the line
        if (rx > sizeof(buffer)) {
            rx = sizeof(buffer);
        }
        uint8_t av = rs485_writeAvail();
        if (rx > av) {
            rx = av;
        }
        // Transfer rx bytes
        if (rx > 0) {
            prot_prim_control_read(buffer, rx);
            rs485_write(false, buffer, rx);
            s_lastTime = timers_get();
        }
    }
    else {
        // Data received?
        uint16_t tx = rs485_readAvail(); 
        if (tx > 0) {
           
            // Read data and push it into IP
            tx = tx > sizeof(buffer) ? sizeof(buffer) : tx;
            rs485_read(buffer, tx);
            s_lastTime = timers_get();
                      
            if (rs485_lastRc9) {
                // Read control char
                switch (buffer[tx - 1]) {
                    case RS485_CCHAR_OVER:
                        // Socket 'over'? Go back to transmit state, the secondary
                        // is about to disengage the line
                        rs485_write(0, NULL, 0);
                        rs485_master = 1;
                        over = 1;
                        break;
                    case RS485_CCHAR_IDLE:
                        // Ok, skip the char and keep timer going
                        break;
                    case RS485_CCHAR_CLOSE:
                        // Socket closed. Now the channel is idle again
                        s_busState = BUS_PRIV_STATE_IDLE;
                        s_socketConnected = SOCKET_NOT_CONNECTED;
                        // Don't close the socket, graceful close from the slave, close byte sent
                        over = 1;
                        break;
                    default:
                        // Socket abruptly closed. Now the channel is idle again
                        s_busState = BUS_PRIV_STATE_IDLE;
                        s_socketConnected = SOCKET_NOT_CONNECTED;
                        // Close TCP as well
                        close = 1;
                        over = 1;
                        break;
                }

                // Don't transmit the last control char
                tx--;
            }

            if (tx > 0) {
                prot_prim_control_write(buffer, tx);
            }
            if (over) {
                // Flush
                prot_prim_control_over();
            }
            if (close) {
                // and close?
                prot_prim_control_close();
            }
        }
    }
}

int bus_prim_getChildrenMaskSize() {
    return BUFFER_MASK_SIZE;
}

const uint8_t* bus_prim_getChildrenMask() {
    return bus_prim_knownChildren;
}
