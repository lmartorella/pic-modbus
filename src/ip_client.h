#ifndef _PROT_H_APP_
#define _PROT_H_APP_

#include "hardware/hw.h"

#ifdef HAS_IP

#if __XC8
#include "Compiler.h"
#include "TCPIPStack/TCPIP.h"
#endif

#if _CONF_RASPBIAN
#include "hardware/ip_raspbian.h"
#endif

void ip_poll();
// Manage poll activities
void ip_prot_init();
// Manage slow timer (heartbeats)
void ip_prot_slowTimer();

#endif

#endif //#ifdef HAS_IP

