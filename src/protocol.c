#include "net/net.h"

static TICK_TYPE s_slowTimer;
__bit prot_slowTimer;

static int8_t s_inReadSink;
static int8_t s_inWriteSink;
// SORTED IN ORDER OF NEEDED BYTES!
static enum { 
    CMD_CLOS, // need 0
    CMD_SINK, // need 0
    CMD_CHIL, // need 0
    CMD_READ, // need 2
    CMD_WRIT, // need 2
    CMD_SELE, // need 2
    CMD_GUID,  // need 16
    CMD_NONE
} s_commandToRun;

__bit prot_registered;

void prot_init() {
    // Align 1sec to now()
    s_slowTimer = timers_get();

    prot_registered = false;
    
    s_commandToRun = CMD_NONE;
    s_inWriteSink = s_inReadSink = -1;
}

#define CLOS_IMPL(IMPL)  \
    prot_ ## IMPL ## _control_write("\x1E", 1); \
    prot_ ## IMPL ## _control_close(); \

/* CLOSE the socket */ \
static void CLOS_prim_command() {
    CLOS_IMPL(prim);
}
static void CLOS_sec_command() {
    CLOS_IMPL(sec);
}

#define SINK_IMPL(IMPL) \
    prot_ ## IMPL ## _control_writeW(SINK_IDS_COUNT); \
    prot_ ## IMPL ## _control_write(SINK_IDS, SINK_IDS_COUNT * 4); \
    /* end of transmission, over to Master */ \
    prot_ ## IMPL ## _control_over(); \

/* 0 bytes to receive*/
static void SINK_prim_command() {
    SINK_IMPL(prim);
}
static void SINK_sec_command() {
    SINK_IMPL(sec);
}

#define GUID_IMPL(IMPL) \
    if (!prot_ ## IMPL ## _control_read(&pers_data.deviceId, sizeof(GUID))) { \
        fatal("GU.u"); \
    } \
    /* Have new GUID! Program it. */ \
    pers_save(); \

/* 16 bytes to receive */
static void GUID_prim_command() {
    GUID_IMPL(prim);
}
static void GUID_sec_command() {
    GUID_IMPL(sec);
}

#define READ_IMPL(IMPL) \
    uint16_t sinkId; \
    if (!prot_ ## IMPL ## _control_readW(&sinkId)) { \
        fatal("RD.u"); \
    } \
    s_inWriteSink = (int8_t)sinkId; \

/* 2 bytes to receive */
static void READ_prim_command() {
    READ_IMPL(prim);
}
static void READ_sec_command() {
    READ_IMPL(sec);
}

#define WRIT_IMPL(IMPL) \
    uint16_t sinkId; \
    if (!prot_ ## IMPL ## _control_readW(&sinkId)) { \
        fatal("WR.u"); \
    } \
    s_inReadSink = (int8_t)sinkId; \

/* 2 bytes to receive */
static void WRIT_prim_command() {
    WRIT_IMPL(prim);
}
static void WRIT_sec_command() {
    WRIT_IMPL(sec);
}

static void SELE_prim_command() {
    // Select subnode. 
    uint16_t w;
    if (!prot_prim_control_readW(&w)) {
        fatal("SE.u");
    }
    
    // Select subnode.
    if (w > 0) {
        // Otherwise connect the socket
        bus_prim_connectSocket(w - 1);
    }
    prot_registered = true;
}

static void SELE_sec_command() {
    // Select subnode. 
    uint16_t w;
    if (!prot_sec_control_readW(&w)) {
        fatal("SE.u");
    }
    
    // Select subnode.
    // Simply ignore when no subnodes
    prot_registered = true;
}

// 0 bytes to receive
static void CHIL_prim_command()
{
    // Fetch my GUID
    // Send ONLY mine guid. Other GUIDS should be fetched using SELE first.
    prot_prim_control_write(&pers_data.deviceId, sizeof(GUID));
    
    // Propagate the request to all children to fetch their GUIDs
    uint16_t count = bus_prim_getChildrenMaskSize();
    prot_prim_control_writeW(count);
    prot_prim_control_write(bus_prim_getChildrenMask(), count);

    bus_prim_resetDirtyChildren();
    
    // end of transmission, over to Master
    prot_prim_control_over();
}

// 0 bytes to receive
static void CHIL_sec_command()
{
    // Fetch my GUID
    // Send ONLY mine guid. Other GUIDS should be fetched using SELE first.
    prot_sec_control_write(&pers_data.deviceId, sizeof(GUID));
    
    // No children
    uint16_t count = 0;
    prot_sec_control_writeW(count);
    
    // end of transmission, over to Master
    prot_sec_control_over();
}

// Code-memory optimized for small PIC XC8
static _Bool memcmp2(char c1, char c2, char d1, char d2) {
    return d1 == c1 && d2 == c2;
}

#define POLL_IMPL(IMPL) \
    if (s_inReadSink >= 0) { \
        /* Tolerates empty rx buffer */ \
        _Bool again = sink_readHandlers[s_inReadSink](); \
        if (!again) { \
            s_inReadSink = -1; \
        } \
        return true; \
    } \
    if (s_inWriteSink >= 0) { \
        /* Address sink */ \
        _Bool again = sink_writeHandlers[s_inWriteSink](); \
        if (!again){ \
            s_inWriteSink = -1; \
            /* end of transmission, over to Master */ \
            prot_ ## IMPL ## _control_over(); \
        } \
        return true; \
    } \
    \
    uint8_t s = prot_ ## IMPL ## _control_readAvail(); \
    if (s_commandToRun != CMD_NONE) { \
        uint8_t needed = 2; \
        if (s_commandToRun <= CMD_CHIL) { \
            needed = 0; \
        } else if (s_commandToRun == CMD_GUID) { \
            needed = 16; \
        } \
        if (s >= needed) { \
            CLRWDT(); \
            switch (s_commandToRun) { \
                case CMD_READ: \
                    READ_ ## IMPL ## _command(); \
                    break; \
                case CMD_WRIT: \
                    WRIT_ ## IMPL ## _command(); \
                    break; \
                case CMD_CLOS: \
                    CLOS_ ## IMPL ## _command(); \
                    break; \
                case CMD_SELE: \
                    SELE_ ## IMPL ## _command(); \
                    break; \
                case CMD_SINK: \
                    SINK_ ## IMPL ## _command(); \
                    break; \
                case CMD_CHIL: \
                    CHIL_ ## IMPL ## _command(); \
                    break; \
                case CMD_GUID: \
                    GUID_ ## IMPL ## _command(); \
                    break; \
            } \
            s_commandToRun = CMD_NONE; \
        } \
        return true; \
    } else { \
        /* So decode message then */ \
        /* Minimum msg size */ \
        if (s >= sizeof(TWOCC)) { \
            /* This can even peek only one command. \
             Until not closed by server, or CLOS command sent, the channel can stay open. */ \
            TWOCC msg; \
            prot_ ## IMPL ## _control_read(&msg, sizeof(TWOCC)); \
            \
            if (memcmp2('R', 'D', msg.chars.c1, msg.chars.c2)) { \
                s_commandToRun = CMD_READ; \
            } else if (memcmp2('W', 'R', msg.chars.c1, msg.chars.c2)) {  \
                s_commandToRun = CMD_WRIT; \
            } else if (memcmp2('C', 'L', msg.chars.c1, msg.chars.c2)) { \
                s_commandToRun = CMD_CLOS; \
            } else if (memcmp2('S', 'L', msg.chars.c1, msg.chars.c2)) { \
                s_commandToRun = CMD_SELE; \
            } else if (memcmp2('S', 'K', msg.chars.c1, msg.chars.c2)) { \
                s_commandToRun = CMD_SINK; \
            } else if (memcmp2('C', 'H', msg.chars.c1, msg.chars.c2)) { \
                s_commandToRun = CMD_CHIL; \
            } else if (memcmp2('G', 'U', msg.chars.c1, msg.chars.c2)) { \
                s_commandToRun = CMD_GUID; \
            } else { \
                /* Unknown command */ \
                fatal("CM.u"); \
            } \
            return true; \
        } else { \
            /* Otherwise wait for data */ \
            return false; \
        } \
    } \

static _Bool pollProtocol_prim() {
    POLL_IMPL(prim);
}
static _Bool pollProtocol_sec() {
    POLL_IMPL(sec);
}


/**
 * Manage POLLs (read buffers)
 */
_Bool prot_prim_poll() {
    prot_slowTimer = 0;
    CLRWDT();

    TICK_TYPE now = timers_get();
    if (now - s_slowTimer >= TICKS_PER_SECOND)
    {
        s_slowTimer = now;
        prot_slowTimer = 1;
        ip_prot_slowTimer();
    }
    ip_poll();
    
    if (!prot_prim_control_isConnected()) {
        bus_prim_disconnectSocket(SOCKET_ERR_CLOSED_BY_PARENT);
        return false;
    }

    // Socket connected?
    switch (bus_prim_getState()) {
        case BUS_STATE_SOCKET_CONNECTED:
            // TCP is still polled by bus
            return true;
        case BUS_STATE_SOCKET_TIMEOUT:
            // drop the TCP connection        
            prot_control_abort();
            break;
        case BUS_STATE_SOCKET_FRAME_ERR:
            // drop the TCP connection        
            prot_control_abort();
            break;
    }
    return pollProtocol_prim();
}

/**
 * Manage POLLs (read buffers)
 */
_Bool prot_sec_poll() {
    prot_slowTimer = 0;
    CLRWDT();

    TICK_TYPE now = timers_get();
    if (now - s_slowTimer >= TICKS_PER_SECOND)
    {
        s_slowTimer = now;
        prot_slowTimer = 1;
    }
    
    if (!prot_sec_control_isConnected()) {
        return false;
    }
    return pollProtocol_sec();
}
