#ifndef _PROT_H_APP_
#define _PROT_H_APP_

/**
 * Module to implement a master node (IP client)
 */

#include "hardware/hw.h"

#ifdef HAS_IP

#if __XC8
#include "tcpipstack/Include/Compiler.h"
#include "tcpipstack/Include/TCPIPStack/TCPIP.h"
#endif

#if _CONF_LINUX
#include "hardware/linux/ip.h"
#endif

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

#endif

#endif //#ifdef HAS_IP

