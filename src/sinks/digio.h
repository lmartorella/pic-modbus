#ifndef DIGIO_H
#define	DIGIO_H

#if defined(HAS_DIGIO_OUT) || defined(HAS_DIGIO_IN)
#define DIGIO_IN_SINK_ID "DIAR"
#define DIGIO_OUT_SINK_ID "DOAR"
void digio_init();
#endif

#if defined(HAS_DIGIO_IN)
bit digio_in_write();
#endif

#if defined(HAS_DIGIO_OUT)
bit digio_out_read();
bit digio_out_write();
#endif

#endif	/* DIGIO_H */

