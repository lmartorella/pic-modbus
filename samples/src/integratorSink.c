#include <pic-modbus/modbus.h>
#include "integratorSink.h"
#include "../hw/an_integrator.h"

// Uses RB1, range from 0V to 1.024V
#define INIT_ANALOG_INTEGRATOR() \
    ANSELBbits.ANSB1 = 1;   \
    TRISBbits.TRISB1 = 1;   \
    FVRCONbits.ADFVR = 1;   \
    FVRCONbits.CDAFVR = 0;  \
    FVRCONbits.FVREN = 1;   \
    while (!FVRCONbits.FVRRDY); \
    ADCON0bits.CHS = 11;    \
    ADCON1bits.ADNREF = 0;  \
    ADCON1bits.ADPREF = 3;  \

// The transfer message
typedef struct {
    // A float to convert A/D integer to unit (server should do it)
    float factor;
    // The data
    ANALOG_INTEGRATOR_DATA data;
} SINK_MSG;
// Ensure float are 32bits
//static char x[sizeof(float) - 4];

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
