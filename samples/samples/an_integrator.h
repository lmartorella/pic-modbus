#ifndef _AN_INTEGRATOR_H
#define _AN_INTEGRATOR_H

// 1A = 1mA, on 39ohm = 39mV, sampled against 1.024V/1024 = 1/39 of the scale
#define ANALOG_INTEGRATOR_FACTOR (1.0f/39.0f)

typedef struct {
    // The integrated A/D value for the last period. Single reading is unsigned 10bits.
    uint32_t value;
    // The count of samles part of the reading
    uint16_t count;
} ANALOG_INTEGRATOR_DATA;

void anint_init(void);
void anint_poll(void);

// Read data and reset counters
void anint_read(ANALOG_INTEGRATOR_DATA* data);

#endif
