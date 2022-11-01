#ifndef _PROT_H_APP_
#define _PROT_H_APP_

/**
 * Module to implement a master node (IP client)
 */

/**
 * Poll activities
 */
void ip_poll();

/**
 * Initialize the IP module
 */
void ip_prot_init();

/**
 * Called a slow timer (1 sec), for heartbeats
 */
void ip_prot_slowTimer();

/**
 * Flush socket data
 */
void ip_flush();

__bit ip_isConnected();

/**
 * Do a select on the socket
 */ 
void ip_waitEvent();

#endif //#ifdef HAS_IP

