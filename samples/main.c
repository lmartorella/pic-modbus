#include <net/net.h>
#include "./samples.h"

void __interrupt(PRIO_TYPE) low_isr() {
    // Update tick timers at ~Khz freq
    timers_poll();
#ifdef BEAN_INTERRUPT_VECTOR
    dcnt_interrupt();
#endif
}

void main() {
    // Analyze RESET reason
    sys_init();

    net_cl_init();
                
    sinks_init();
        
    sys_enableInterrupts();

    // I'm alive
    while (1) {
        CLRWDT();
        net_cl_poll();
        sinks_poll();
    }
}

