#include "../../../src/nodes/pch.h"
#include "../../../src/nodes/sinks.h"
#include "../../../src/nodes/protocol.h"    
#include "integratorSink.h"
#include "../hardware/an_integrator.h"

#ifdef HAS_ANALOG_INTEGRATOR

// The transfer message
typedef struct {
    // A float to convert A/D integer to unit (server should do it)
    float factor;
    // The data
    ANALOG_INTEGRATOR_DATA data;
} SINK_MSG;
// Ensure float are 32bits
static char x[sizeof(float) - 4];

__bit anint_sinkWrite() {
    if (prot_control_writeAvail() < sizeof(SINK_MSG)) {
        // Still data to transfer
        return 1;
    }
    SINK_MSG msg;
    msg.factor = ANALOG_INTEGRATOR_FACTOR;
    anint_read(&msg.data);
    
    // Send data
    prot_control_write(&msg, sizeof(msg));
    return 0;
}

#endif