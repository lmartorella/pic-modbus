#include "../pch.h"
#include "digio.h"
#include "../protocol.h"

#if defined(HAS_DIGIO_IN) || defined(HAS_DIGIO_OUT)

// Only support 1 bit for IN and 1 for OUT (can even be the same)

void digio_init()
{
#ifdef HAS_DIGIO_IN
    // First enable input bits
    DIGIO_TRIS_IN_BIT = 1;
#endif
#ifdef HAS_DIGIO_OUT
    // Then the output. So if the same port is configured as I/O it will work
    DIGIO_TRIS_OUT_BIT = 0;
#endif
}

#endif

#ifdef HAS_DIGIO_OUT

bit digio_out_write()
{
    // One port
    WORD b = 1;
    // Number of switch = 1
    prot_control_write(&b, sizeof(WORD));
    return FALSE;
}

// Read bits to set as output
bit digio_out_read()
{
    if (prot_control_readAvail() < 2) {
        // Need more data
        return TRUE;
    }
    BYTE arr;
    // Number of bytes sent (expect 1)
    prot_control_read(&arr, 1);
    // The byte: the bit 0 is data
    prot_control_read(&arr, 1);
    DIGIO_PORT_OUT_BIT = !!arr;
    return FALSE;
}

#endif
#ifdef HAS_DIGIO_IN

// Write bits read as input
bit digio_in_write()
{   
    BYTE swCount = 1;
    // Number of switches sent (1)
    prot_control_write(&swCount, 1);
    BYTE arr = DIGIO_PORT_IN_BIT;
    // The byte: the bit 0 is data
    prot_control_write(&arr, 1);
    return FALSE;
}

#endif
