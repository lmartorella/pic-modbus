#ifndef BUS_SEC_H
#define	BUS_SEC_H

/**
 * Wired bus communication module, for bean nodes
 */

void bus_sec_init();

/**
 * Poll general bus activities
 * Returns true if the node is active and requires polling
 */
__bit bus_sec_poll();

void bus_sec_abort();
__bit bus_sec_isConnected();

#endif	/* BUS_SEC_H */
