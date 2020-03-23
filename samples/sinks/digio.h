#ifndef DIGIO_H
#define	DIGIO_H

// Direct access to digital I/O lines of the MCU

#if defined(HAS_DIGIO_OUT) || defined(HAS_DIGIO_IN)
#define DIGIO_IN_SINK_ID "DIAR"
#define DIGIO_OUT_SINK_ID "DOAR"
void digio_init();
#endif

// Digital input is reading the current state and the recent event log
// Read: read the number of available lines, the current state, and the 
// last unread variations.
#if defined(HAS_DIGIO_IN)
void digio_in_poll();
bit digio_in_write();
#endif

// Digital output is simply setting the state of the bits.
// Read: read the number of available lines
// Write: write the state of the lines (all of them)
#if defined(HAS_DIGIO_OUT)
bit digio_out_read();
bit digio_out_write();
#endif

#endif	/* DIGIO_H */

