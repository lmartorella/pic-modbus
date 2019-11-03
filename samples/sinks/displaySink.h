#ifndef DISPLAYSINK_H
#define	DISPLAYSINK_H

// Drives a 2-lines LCD display (CM1602)

#if defined(HAS_BUS) && (defined(HAS_CM1602) || defined(_CONF_RASPBIAN))

#define SINK_LINE_ID "LINE"
bit line_read();
bit line_write();

#endif

#endif	/* DISPLAYSINK_H */

