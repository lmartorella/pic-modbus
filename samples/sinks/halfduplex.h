#ifndef XC_HFD_TEMPLATE_H
#define	XC_HFD_TEMPLATE_H

// Implement an additional polled UART for MCUs with one UART only (used by bus)

#define SINK_HALFDUPLEX_ID "SLIN"
void halfduplex_init();
__bit halfduplex_read();
__bit halfduplex_write();

#endif	/* XC_HFD_TEMPLATE_H */

