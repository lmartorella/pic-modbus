#include "../../src/nodes/pch.h"
#include "../../src/nodes/ip_client.h"
#include "../../src/nodes/appio.h"
#include "../../src/nodes/persistence.h"
#include "../../src/nodes/protocol.h"
#include "../../src/nodes/sinks.h"

#ifdef INTERRUPT_VECTOR
extern void INTERRUPT_VECTOR();
#endif

void interrupt PRIO_TYPE low_isr()
{
    // Update tick timers at ~Khz freq
    timers_poll();
#ifdef HAS_RS485
    rs485_interrupt();
#endif
#ifdef INTERRUPT_VECTOR
    INTERRUPT_VECTOR();
#endif
}

void main()
{
    // Analyze RESET reason
    sys_storeResetReason();

    // Init Ticks on timer0 (low prio) module
    timers_init();
    io_init();

    pers_load();

#ifdef HAS_RS485_BUS
    prot_init();
#endif

#ifdef HAS_RS485
    rs485_init();
#endif

#ifdef BUSPOWER_PORT
    // Enable bus power to slaves
    BUSPOWER_TRIS = 0;
    BUSPOWER_PORT = 1;
#endif
    
#ifdef INIT_PORTS
    INIT_PORTS();
#endif
            
    sinks_init();
        
    enableInterrupts();

    // I'm alive
    while (1) {   
        CLRWDT();

#if defined(HAS_RS485_BUS_CLIENT) || defined(HAS_RS485_BUS_SERVER)
        bus_poll();
#endif
#ifdef HAS_RS485_BUS
        prot_poll();
#endif
#ifdef HAS_RS485
        rs485_poll();
#endif

        sinks_poll();
        
        pers_poll();
    }
}

