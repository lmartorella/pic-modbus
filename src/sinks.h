#ifndef SINKS_H_
#define SINKS_H_

#include "hardware/hw.h"

#ifdef HAS_BUS

void sinks_init();

#define SINK_SYS_ID "SYS "
bit sys_read();
bit sys_write();

// Returns 1 if more data should follow, 0 if finished
// The 'write' function sends data to master. The 'read' function receives data from master.
typedef bit (*SinkFunction)();

#ifdef HAS_FIRMWARE
typedef struct {
    char ID[4];
    const SinkFunction const readPtr;
    const SinkFunction const writePtr;
} SINK_DESC;

typedef struct {
    WORD sinkCount;
    SINK_DESC sinks[];
} FIRMWARE_HEADER;

// Firmware location
extern const FIRMWARE_HEADER e_header;

#else
extern const char* const SINK_IDS;
extern const int SINK_IDS_COUNT;
extern const SinkFunction const sink_readHandlers[];
extern const SinkFunction const sink_writeHandlers[];
#endif

#endif
#endif
