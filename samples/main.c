#include <net/net.h>
#include "./samples.h"

#ifdef PROTO_PINOUT
// 17008 is the debug port
#define SERVER_CONTROL_UDP_PORT 17008
#else
// 17007 is the release port
#define SERVER_CONTROL_UDP_PORT 17007
#endif

void __interrupt(PRIO_TYPE) low_isr() {
    // Update tick timers at ~Khz freq
    timers_poll();
#ifdef BEAN_INTERRUPT_VECTOR
    dcnt_interrupt();
#endif
}

void main()
{
    // Analyze RESET reason
    sys_storeResetReason();

    net_init();

#ifdef BUSPOWER_PORT
    // Enable bus power to slaves
    BUSPOWER_TRIS = 0;
    BUSPOWER_PORT = 1;
#endif
    
#ifdef INIT_PORTS
    INIT_PORTS();
#endif
            
    sinks_init();
        
    sys_enableInterrupts();

    // I'm alive
    while (1) {
        rs485_poll();
        net_poll();
        sinks_poll();      
    }
}

