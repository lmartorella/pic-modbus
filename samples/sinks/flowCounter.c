#include "../../../src/nodes/pch.h"
#include "../../../src/nodes/protocol.h"
#include "../hardware/counter.h"
#include "flowCounter.h"

#ifdef HAS_DIGITAL_COUNTER

bit flow_write() {
    // Write counter
    if (prot_control_writeAvail() < sizeof(DCNT_DATA)) {
        fatal("FL_U");
    }

    DCNT_DATA data;
    dcnt_getDataCopy(&data);
    prot_control_write(&data, sizeof(DCNT_DATA));

    // Done
    return FALSE;
}

#endif