#include "../appio.h"
#include "../hardware/fuses.h"
#include "../hardware/cm1602.h"
#include <stdio.h>

#define GetPeripheralClock() (25000000/4)
#define TICKS_PER_SECOND     ((GetPeripheralClock()+128ull)/256ull)	// Internal core clock drives timer with 1:256 prescaler
#define TICK_SECOND          ((QWORD)TICKS_PER_SECOND)

static DWORD s_1sec;
static DWORD s_10msec;
#define TICKS_1S ((DWORD)TICK_SECOND)
#define TICKS_10MS (DWORD)(TICKS_1S / 100)

static volatile DWORD dwInternalTicks = 0;

void interrupt low_priority low_isr(void)
{
    if (INTCONbits.TMR0IF)
    {
        // Increment internal high tick counter
        dwInternalTicks++;

        // Reset interrupt flag
        INTCONbits.TMR0IF = 0;
    }
}

DWORD TickGet(void);

void TickInit(void)
{
    // Use Timer0 for 8 bit processors
    // Initialize the time
    TMR0H = 0;
    TMR0L = 0;

    // Set up the timer interrupt
    INTCON2bits.TMR0IP = 0;		// Low priority
    INTCONbits.TMR0IF = 0;
    INTCONbits.TMR0IE = 1;		// Enable interrupt

    // Timer0 on, 16-bit, internal timer, 1:256 prescalar
    T0CON = 0x87;

    s_1sec = s_10msec = TickGet();
}

DWORD TickGet(void)
{
    // 6-byte value to store Ticks.  Allows for use over longer periods of time.
    BYTE vTickReading[6];
    // Perform an Interrupt safe and synchronized read of the 48-bit
    // tick value
    do
    {
        INTCONbits.TMR0IE = 1;		// Enable interrupt
        Nop();
        INTCONbits.TMR0IE = 0;		// Disable interrupt
        vTickReading[0] = TMR0L;
        vTickReading[1] = TMR0H;
        *((DWORD*)&vTickReading[2]) = dwInternalTicks;
    } while(INTCONbits.TMR0IF);
    INTCONbits.TMR0IE = 1;			// Enable interrupt
    return *((DWORD*)&vTickReading[0]);
}

typedef union
{
    struct
    {
        unsigned timer_1s: 1;
        unsigned timer_10ms: 1;
    };
    BYTE v;
} TIMER_RES;

TIMER_RES timers_check()
{
    TIMER_RES res;
    res.v = 0;
    
    DWORD now = TickGet();
    if ((now - s_1sec) >= TICKS_1S)
    {
        s_1sec = now;
        res.timer_1s = 1;
    }
    if ((now - s_10msec) >= TICKS_10MS)
    {
        s_10msec = now;
        res.timer_10ms = 1;
    }
    return res;
}

void main()
{
    cm1602_reset();
    cm1602_clear();
    cm1602_setEntryMode(MODE_INCREMENT | MODE_SHIFTOFF);
    cm1602_enable(ENABLE_DISPLAY | ENABLE_CURSOR | ENABLE_CURSORBLINK);

    char buffer[16];
    TickInit();
    int slow = 0;
    int fast = 0;
    while (1)
    {
        ClrWdt();
        TIMER_RES res = timers_check();
        if (res.timer_1s)
        {
            sprintf(buffer, "SL:%d", slow++);
            println(buffer);
        }
        if (res.timer_10ms)
        {
            sprintf(buffer, "FA:%d", fast++);
            printlnUp(buffer);
        }
    }
}
