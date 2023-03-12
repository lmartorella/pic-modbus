#ifndef DIGIO_H
#define	DIGIO_H

// Direct access to digital I/O lines of the MCU

#define DIGIO_IN_SINK_ID "DIAR"
#define DIGIO_OUT_SINK_ID "DOAR"
void digio_init();

// Digital input is reading the current state and the recent event log
// Read: read the number of available lines, the current state, and the 
// last unread variations.
void digio_in_poll();
__bit digio_in_write();

// Digital output is simply setting the state of the bits.
// Read: read the number of available lines
// Write: write the state of the lines (all of them)
__bit digio_out_read();
__bit digio_out_write();

#endif	/* DIGIO_H */

