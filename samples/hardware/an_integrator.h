#ifndef _AN_INTEGRATOR_H
#define _AN_INTEGRATOR_H

#ifdef HAS_ANALOG_INTEGRATOR

typedef struct {
    // The integrated A/D value for the last period. Single reading is unsigned 10bits.
    uint32_t value;
    // The count of samles part of the reading
    uint16_t count;
} ANALOG_INTEGRATOR_DATA;

void anint_init();
void anint_poll();

// Read data and reset counters
void anint_read(ANALOG_INTEGRATOR_DATA* data);

#endif
#endif
