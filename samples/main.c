#include <net/net.h>
#include "./samples.h"

#ifdef BEAN_INTERRUPT_VECTOR
extern void BEAN_INTERRUPT_VECTOR();
#endif

void __interrupt(PRIO_TYPE) low_isr()
{
    // Update tick timers at ~Khz freq
    timers_poll();
    rs485_interrupt();
#ifdef BEAN_INTERRUPT_VECTOR
    BEAN_INTERRUPT_VECTOR();
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
        net_poll();
        sinks_poll();      
    }
}

