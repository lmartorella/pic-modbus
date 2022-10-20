#ifndef PROTOCOL_H
#define	PROTOCOL_H

/**
 * Protocol implementation module
 */

#include "bus.h"
#ifdef HAS_RS485
#include "rs485.h"
#endif

#ifdef HAS_RS485_BUS

#ifdef HAS_RS485_BUS_CLIENT
// Directly with define in order to minimize stack usage
#define prot_control_readW(w) rs485_read((BYTE*)w, 2) 
#define prot_control_read(data, size) rs485_read((BYTE*)data, size)
#define prot_control_writeW(w) rs485_write(FALSE, (BYTE*)&w, 2)
#define prot_control_write(data, size) rs485_write(FALSE, (BYTE*)data, size)
#define prot_control_over() set_rs485_over()
#define prot_control_idle(buf) rs485_write(TRUE, buf, 1)
#define prot_control_readAvail() rs485_readAvail()
#define prot_control_writeAvail() rs485_writeAvail()
#else
bit prot_control_readW(WORD* w);
bit prot_control_read(void* data, WORD size);
void prot_control_writeW(WORD w);
void prot_control_write(const void* data, WORD size);
void prot_control_over();
#define prot_control_idle()
WORD prot_control_readAvail();
WORD prot_control_writeAvail();
extern bit prot_registered;
#endif

void prot_control_close();
void prot_control_abort();
bit prot_control_isConnected();

void prot_init();
void prot_poll();
// Has the slow timer ticked?
extern bit prot_slowTimer;

#endif

#endif	/* PROTOCOL_H */

