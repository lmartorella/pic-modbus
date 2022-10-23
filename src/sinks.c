#include "pch.h"
#include "sinks.h"
#include "appio.h"
#include "protocol.h"
#include "bus_primary.h"

#ifdef HAS_RS485_BUS

const TWOCC ResetCode = { "RS" };
const TWOCC ExceptionText = { "EX" };
const TWOCC EndOfMetadataText = { "EN" };
#ifdef HAS_RS485_BUS_PRIMARY
const TWOCC BusMasterStats = { "BM" };
#endif

enum SYSSINK_CMD {
    SYSSINK_CMD_RESET = 1,
    SYSSINK_CMD_CLRRST = 2,
};

bit sys_read()
{
    if (prot_control_readAvail() < 1) {
        // Wait cmd
        return 1;
    }
    BYTE cmd;
    prot_control_read(&cmd, 1);
    switch (cmd) {
        case SYSSINK_CMD_RESET:
            // Reset device (not as MClr zeroconf)
            fatal("RST");
            break;
        case SYSSINK_CMD_CLRRST:
            // Reset reset reason
            g_resetReason = RESET_NONE;
            break;
    }
    // No more data
    return FALSE;
}

bit sys_write()
{
    WORD l = g_resetReason;
    // Write reset reason
    prot_control_write(&ResetCode, sizeof(TWOCC));
    prot_control_writeW(l);
    
    if (g_resetReason == RESET_EXC)
    {
        prot_control_write(&ExceptionText, sizeof(TWOCC));

        const char *exc = (const char *)g_lastException;
        l = strlen(exc);
        if (l > 12) {
            l = 12;
        }
        prot_control_writeW(l);
        prot_control_write(exc, l);
    }

#ifdef HAS_RS485_BUS_PRIMARY
    prot_control_write(&BusMasterStats, sizeof(TWOCC));
    prot_control_write(&bus_prim_busStats, sizeof(BUS_PRIMARY_STATS));
    memset(&bus_prim_busStats, 0, sizeof(BUS_PRIMARY_STATS));
#endif

    prot_control_write(&EndOfMetadataText, sizeof(TWOCC));
    // Finish
    return FALSE;
}

#endif