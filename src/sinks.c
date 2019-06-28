#include "pch.h"
#include "sinks.h"
#include "appio.h"
#include "protocol.h"

#ifdef HAS_BUS

static bit nil() {
    CLRWDT();
    return FALSE;
}

// Static allocation of sinks
const char* const SINK_IDS = 
    SINK_SYS_ID
#ifdef HAS_DIGIO_OUT
    DIGIO_OUT_SINK_ID
#endif
#ifdef HAS_DIGIO_IN
    DIGIO_IN_SINK_ID
#endif
#ifdef SINK_LINE_ID
    SINK_LINE_ID
#endif
#ifdef HAS_DHT11
    SINK_DHT11_ID
#endif
#if defined(HAS_MAX232_SOFTWARE) || defined(HAS_FAKE_RS232)
    SINK_HALFDUPLEX_ID 
#endif
#if defined(HAS_BMP180)
    SINK_BMP180_ID 
#endif
#if defined(HAS_DIGITAL_COUNTER)
    SINK_FLOW_COUNTER
#endif
#if defined(HAS_CUSTOM_SINK)
    SINK_CUSTOM_ID 
#endif
;

const int SINK_IDS_COUNT = 
    1
#ifdef HAS_DIGIO_OUT
    + 1
#endif
#ifdef HAS_DIGIO_IN
    + 1
#endif
#ifdef SINK_LINE_ID
    + 1
#endif
#ifdef HAS_DHT11
    + 1
#endif
#if defined(HAS_MAX232_SOFTWARE) || defined(HAS_FAKE_RS232)
    + 1
#endif
#if defined(HAS_BMP180)
    + 1
#endif
#if defined(HAS_DIGITAL_COUNTER)
    + 1
#endif
#if defined(HAS_CUSTOM_SINK)
    + 1
#endif
;

const SinkFunction const sink_readHandlers[] = {
    sys_read
#ifdef HAS_DIGIO_OUT
    ,digio_out_read
#endif
#ifdef HAS_DIGIO_IN
    ,nil
#endif
#ifdef SINK_LINE_ID
    ,line_read
#endif
#ifdef HAS_DHT11
    ,nil
#endif
#if defined(HAS_MAX232_SOFTWARE) || defined(HAS_FAKE_RS232)
    ,halfduplex_read 
#endif
#if defined(HAS_BMP180)
    ,bmp180_sinkRead 
#endif
#if defined(HAS_DIGITAL_COUNTER)
    ,nil
#endif
#if defined(HAS_CUSTOM_SINK)
    ,customsink_read 
#endif
};

const SinkFunction const sink_writeHandlers[] = {
    sys_write
#ifdef HAS_DIGIO_OUT
    ,digio_out_write
#endif
#ifdef HAS_DIGIO_IN
    ,digio_in_write
#endif
#ifdef SINK_LINE_ID
    ,line_write
#endif
#ifdef HAS_DHT11
    ,dht11_write
#endif
#if defined(HAS_MAX232_SOFTWARE) || defined(HAS_FAKE_RS232)
    ,halfduplex_write 
#endif
#if defined(HAS_BMP180)
    ,bmp180_sinkWrite 
#endif
#if defined(HAS_DIGITAL_COUNTER)
    ,flow_write
#endif
#if defined(HAS_CUSTOM_SINK)
    ,customsink_write 
#endif
};

const TWOCC ResetCode = { "RS" };
const TWOCC ExceptionText = { "EX" };
const TWOCC EndOfMetadataText = { "EN" };
#ifdef HAS_BUS_SERVER
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
            // Reset device
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
        
        const char *exc = g_lastException;
        l = strlen(exc);
        prot_control_writeW(l);
        prot_control_write(exc, l);
    }

    #ifdef HAS_BUS_SERVER
    prot_control_write(&BusMasterStats, sizeof(TWOCC));
    prot_control_write(&g_busStats, sizeof(BUS_MASTER_STATS));
    memset(&g_busStats, 0, sizeof(BUS_MASTER_STATS));
    #endif

    prot_control_write(&EndOfMetadataText, sizeof(TWOCC));
    // Finish
    return FALSE;
}

#endif