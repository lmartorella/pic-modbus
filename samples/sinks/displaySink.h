#ifndef DISPLAYSINK_H
#define	DISPLAYSINK_H

// Drives a 2-lines LCD display (CM1602)

#if defined(HAS_RS485_BUS) && (defined(HAS_CM1602) || defined(_CONF_LINUX))

#define SINK_LINE_ID "LINE"
__bit line_read();
__bit line_write();

#endif

#endif	/* DISPLAYSINK_H */

