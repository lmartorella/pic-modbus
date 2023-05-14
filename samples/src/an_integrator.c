#include <pic-modbus/modbus.h>
#include "an_integrator.h"

// 1A = 1mA, on 39ohm = 39mV, sampled against 1.024V/1024 = 1/39 of the scale
#define ANALOG_INTEGRATOR_FACTOR (1.0f/39.0f)

static int32_t _accumulator;
static int16_t _count;
static enum {
    IDLE,
    SAMPLING    
} _state;
static TICK_TYPE _lastSample;

void anint_init() {
    _accumulator = 0;
    _count = 0;
    _state = IDLE;
    _lastSample = timers_get();
    
    // Uses RB1, range from 0V to 1.024V
    ANSELBbits.ANSB1 = 1;
    TRISBbits.TRISB1 = 1;
    FVRCONbits.ADFVR = 1;
    FVRCONbits.CDAFVR = 0;
    FVRCONbits.FVREN = 1;
    while (!FVRCONbits.FVRRDY);
    ADCON0bits.CHS = 11;
    ADCON1bits.ADNREF = 0;
    ADCON1bits.ADPREF = 3;
    ADCON1bits.ADFM = 1; // LSB
    ADCON1bits.ADCS = 7; // internal OSC
    NOP();
    ADCON0bits.ADON = 1;
}

// Sample line 4 times per seconds and accumulate value in 32-bit value.
// This means that, at max reading (10 bit), you have 2^(31-10)/4 seconds to read accumulator before overflow (~145h)
// However count is 16 bit signed, so it resets first (after 2^15/4 seconds, ~136 minutes in case of no readings)
void anint_poll() {
    switch (_state) {
        case IDLE: {
            TICK_TYPE now = timers_get();
            if ((now - _lastSample) > (TICKS_PER_SECOND / 4)) {
                _lastSample = now;
                _state = SAMPLING;

                // Start sampling
                ADCON0bits.GO = 1;
            }
            break;
        }
        case SAMPLING: {
            // Check if sample is done
            // if done, read it and 
            if (!ADCON0bits.GO) {
                uint16_t value = (uint16_t)((ADRESH << 8) + ADRESL);
                _accumulator += value;
                _count++;
                if (_accumulator < 0 || _count < 0) {
                    // Overflow
                    fatal("AI_Ov");
                }
                _state = IDLE;
            }            
        }
    }
}

void anint_read(ANALOG_INTEGRATOR_DATA* data) {
    data->count = (uint16_t)_count;
    data->value = (uint32_t)_accumulator;
    _count = 0;
    _accumulator = 0;
}
