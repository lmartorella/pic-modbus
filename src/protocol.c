#include "pch.h"
#include "protocol.h"
#include "appio.h"
#include "persistence.h"
#include "sinks.h"
#include "bus_primary.h"
#include "bus_secondary.h"

#ifdef HAS_RS485_BUS

#ifdef HAS_IP
#include "ip_client.h"
#endif

static TICK_TYPE s_slowTimer;
bit prot_slowTimer;

static signed char s_inReadSink;
static signed char s_inWriteSink;
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

bit prot_registered;

void prot_init()
{
    // Align 1sec to now()
    s_slowTimer = timers_get();

#ifdef HAS_IP
    ip_prot_init();
#else
    io_println("No IP");
#endif

#ifdef HAS_RS485_BUS_PRIMARY
    bus_prim_init();
#elif defined HAS_RS485_BUS_SECONDARY
    bus_sec_init();
#endif
    
    prot_registered = FALSE;
    
    s_commandToRun = CMD_NONE;
    s_inWriteSink = s_inReadSink = -1;

#ifdef DEBUGMODE
    io_printChDbg('K');
#endif
}

static void CLOS_command()
{
#ifdef DEBUGMODE
    io_printChDbg('l');
#endif
    // CLOSE the socket
    prot_control_write("\x1E", 1);
    prot_control_close();
}

static void SELE_command()
{
#ifdef DEBUGMODE
    io_printChDbg('s');
#endif
    // Select subnode. 
    WORD w;
    if (!prot_control_readW(&w)) {
        fatal("SE.u");
    }
    
    // Select subnode.
    // Simply ignore when no subnodes
#ifdef HAS_RS485_BUS_PRIMARY
    if (w > 0)
    {
        // Otherwise connect the socket
        bus_prim_connectSocket(w - 1);
    }
#endif
    prot_registered = TRUE;
}

// 0 bytes to receive
static void CHIL_command()
{
#ifdef DEBUGMODE
    io_printChDbg('c');
#endif
    // Fetch my GUID
    // Send ONLY mine guid. Other GUIDS should be fetched using SELE first.
    prot_control_write(&pers_data.deviceId, sizeof(GUID));
    
#ifdef HAS_RS485_BUS_PRIMARY
    // Propagate the request to all children to fetch their GUIDs
    WORD count = bus_prim_getChildrenMaskSize();
    prot_control_writeW(count);
    prot_control_write(bus_prim_getChildrenMask(), count);

    bus_prim_resetDirtyChildren();
#else    
    // No children
    WORD count = 0;
    prot_control_writeW(count);
#endif
    
    // end of transmission, over to Master
    prot_control_over();
}

// 0 bytes to receive
static void SINK_command()
{
#ifdef DEBUGMODE
    io_printChDbg('k');
#endif
    
    prot_control_writeW(SINK_IDS_COUNT);
    prot_control_write(SINK_IDS, SINK_IDS_COUNT * 4);

    // end of transmission, over to Master
    prot_control_over();
}

// 16 bytes to receive
static void GUID_command()
{
#ifdef DEBUGMODE
    io_printChDbg('g');
#endif
    if (!prot_control_read(&pers_data.deviceId, sizeof(GUID))) {
        fatal("GU.u");
    }
    // Have new GUID! Program it.
    pers_save();   
}

// 2 bytes to receive
static void READ_command()
{
#ifdef DEBUGMODE
    io_printChDbg('r');
#endif
    WORD sinkId;
    if (!prot_control_readW(&sinkId))
    {
        fatal("RD.u");
    }   
    s_inWriteSink = sinkId;
}

// 2 bytes to receive
static void WRIT_command()
{
#ifdef DEBUGMODE
    io_printChDbg('w');
#endif
    WORD sinkId;
    if (!prot_control_readW(&sinkId))
    {
        fatal("WR.u");
    }
    s_inReadSink = sinkId;
}

// Code-memory optimized for small PIC XC8
static bit memcmp2(char c1, char c2, char d1, char d2) {
    return d1 == c1 && d2 == c2;
}

/*
    Manage POLLs (read buffers)
*/
void prot_poll()
{
    prot_slowTimer = 0;
    CLRWDT();
#ifdef HAS_IP
    // Do ETH stuff
    StackTask();
    // This tasks invokes each of the core stack application tasks
    StackApplications();
#endif

    TICK_TYPE now = timers_get();
    if (now - s_slowTimer >= TICKS_PER_SECOND)
    {
        s_slowTimer = now;
        prot_slowTimer = 1;
#ifdef HAS_IP
        ip_prot_slowTimer();
#endif
    }
#ifdef HAS_IP
    ip_poll();
#endif
    
    if (!prot_control_isConnected()) {
#ifdef HAS_RS485_BUS_PRIMARY
        bus_prim_disconnectSocket(SOCKET_ERR_CLOSED_BY_PARENT);
#endif
        return;
    }

#ifdef HAS_RS485_BUS_PRIMARY
    // Socket connected?
    switch (bus_prim_getState()) {
        case BUS_STATE_SOCKET_CONNECTED:
            // TCP is still polled by bus
            return;
        case BUS_STATE_SOCKET_TIMEOUT:
            // drop the TCP connection        
            prot_control_abort();
            break;
        case BUS_STATE_SOCKET_FRAME_ERR:
            // drop the TCP connection        
            prot_control_abort();
            break;
    }
#endif

    if (s_inReadSink >= 0) {
        // Tolerates empty rx buffer
        BOOL again = sink_readHandlers[s_inReadSink]();
        if (!again) {
            s_inReadSink = -1;
        }
        return;
    }
    if (s_inWriteSink >= 0) {
        // Address sink
        BOOL again = sink_writeHandlers[s_inWriteSink]();
        if (!again){
            s_inWriteSink = -1;
            // end of transmission, over to Master
            prot_control_over();
        }
        return;
    }

    BYTE s = prot_control_readAvail();
    if (s_commandToRun != CMD_NONE) {
        BYTE needed = 2;
        if (s_commandToRun <= CMD_CHIL) {
            needed = 0;
        } else if (s_commandToRun == CMD_GUID) {
            needed = 16;
        }
        if (s >= needed) {
            CLRWDT();
            switch (s_commandToRun) {
                case CMD_READ:
                    READ_command();
                    break;
                case CMD_WRIT:
                    WRIT_command();
                    break;
                case CMD_CLOS:
                    CLOS_command();
                    break;
                case CMD_SELE:
                    SELE_command();
                    break;
                case CMD_SINK:
                    SINK_command();
                    break;
                case CMD_CHIL:
                    CHIL_command();
                    break;
                case CMD_GUID:
                    GUID_command();
                    break;
            }
            s_commandToRun = CMD_NONE;
        }
    }
    else {
        // So decode message then
        if (s >= sizeof(TWOCC)) // Minimum msg size
        {
            // This can even peek only one command.
            // Until not closed by server, or CLOS command sent, the channel can stay open.

            TWOCC msg;
            prot_control_read(&msg, sizeof(TWOCC));

            if (memcmp2('R', 'D', msg.chars.c1, msg.chars.c2)) { 
                s_commandToRun = CMD_READ;
            } else if (memcmp2('W', 'R', msg.chars.c1, msg.chars.c2)) { 
                s_commandToRun = CMD_WRIT;
            } else if (memcmp2('C', 'L', msg.chars.c1, msg.chars.c2)) { 
                s_commandToRun = CMD_CLOS;
            } else if (memcmp2('S', 'L', msg.chars.c1, msg.chars.c2)) { 
                s_commandToRun = CMD_SELE;
            } else if (memcmp2('S', 'K', msg.chars.c1, msg.chars.c2)) { 
                s_commandToRun = CMD_SINK;
            } else if (memcmp2('C', 'H', msg.chars.c1, msg.chars.c2)) { 
                s_commandToRun = CMD_CHIL;
            } else if (memcmp2('G', 'U', msg.chars.c1, msg.chars.c2)) { 
                s_commandToRun = CMD_GUID;
            } else {
                // Unknown command
                fatal("CM.u");
            }
        }
        // Otherwise wait for data
    }
}

#endif