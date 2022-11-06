#ifndef _PROT_H_APP_
#define _PROT_H_APP_

/**
 * Module to implement a master node (IP client)
 */

/**
 * Poll activities
 */
void ip_poll(void);

/**
 * Initialize the IP module
 */
void ip_init(uint16_t serverUdpPort);

/**
 * Called a slow timer (1 sec), for heartbeats
 */
void ip_slowTimer(void);

/**
 * Flush socket data
 */
void ip_flush(void);

__bit ip_isConnected(void);

/**
 * Do a select on the socket
 */ 
void ip_waitEvent(void);

#endif //#ifdef HAS_IP

