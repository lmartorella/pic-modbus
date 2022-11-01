#include <net/net.h>

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

    // Init Ticks on timer0 (low prio) module
    timers_init();
    io_init();
    led_init();

    pers_load();

    prot_init();

    rs485_init();

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
        CLRWDT();

#if defined(HAS_RS485_BUS_SECONDARY)
        bus_sec_poll();
#endif
#if defined(HAS_RS485_BUS_PRIMARY)
        bus_prim_poll();
#endif
        prot_poll();
        rs485_poll();

        sinks_poll();
        
        pers_poll();
    }
}

