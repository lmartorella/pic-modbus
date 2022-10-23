#ifndef BUS_SEC_H
#define	BUS_SEC_H

/**
 * Wired bus communication module (both master on bean nodes)
 */

#ifdef HAS_RS485_BUS_SECONDARY

void bus_sec_init();
// Poll general bus activities
void bus_sec_poll();
bit bus_sec_isIdle();

#endif

#endif	/* BUS_H */

