#ifndef XC_HFD_TEMPLATE_H
#define	XC_HFD_TEMPLATE_H

#if defined(HAS_BUS) && defined(HAS_MAX232_SOFTWARE)

#define SINK_HALFDUPLEX_ID "SLIN"
void halfduplex_init();
bit halfduplex_read();
bit halfduplex_write();

#endif

#endif	/* XC_HFD_TEMPLATE_H */

