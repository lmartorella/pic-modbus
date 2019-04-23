#ifndef FUSES_TEMP_AND_VALVE_H
#define	FUSES_TEMP_AND_VALVE_H

// Installed one for each floor to control room temperature and hot water control

#define HAS_DHT11
#define DHT11_PORT_PULLUPS_INIT() { WPUB = 0x00; WPUBbits.WPUB3 = 1; OPTION_REGbits.nWPUEN = 0; ANSELBbits.ANSB3 = 0; }
#define DHT11_PORT_TRIS TRISBbits.TRISB3
#define DHT11_PORT PORTBbits.RB3
#define US_TIMER TMR1L
    // Prescaler 1:1, = 1MHz timer (us), started
#define US_TIMER_INIT() { T1CON = 1; }

#define HAS_DIGIO_IN
#define DIGIO_TRIS_IN_BIT TRISBbits.TRISB0
#define DIGIO_PORT_IN_BIT PORTBbits.RB0

#endif	/* FUSES_TEMP_AND_VALVE_H */

