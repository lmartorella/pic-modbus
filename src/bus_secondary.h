#ifndef BUS_SEC_H
#define	BUS_SEC_H

/**
 * Wired bus communication module, for bean nodes
 */

void bus_sec_init();
// Poll general bus activities
void bus_sec_poll();
__bit bus_sec_isIdle();
__bit bus_sec_isConnected();

#endif	/* BUS_SEC_H */
