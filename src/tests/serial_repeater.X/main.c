#include "../../pch.h"
#include "ext_config.h"
#include "../../appio.h"
#include "../../hardware/tick.h"
#include "../../rs485.h"

// ==== ETH CARD based on 18f87j60

// For the list of fuses, run "mcc18.exe --help-config -p=18f87j60"

// Enable WDT, 1:256 prescaler
#pragma config WDT = ON
#pragma config WDTPS = 256

// Enable stack overflow reset en bit
#pragma config STVR = ON

// Microcontroller mode ECCPA/P2A multiplexed (not used!)
//#pragma config ECCPMX = 1/0
//#pragma config CCP2MX = 1/0

// Ethernet led enabled. RA0/1 are multiplexed with LEDA/LEDB
#pragma config ETHLED = ON

// Fail-safe clock monitor enabled (switch to internal oscillator in case of osc failure)
#pragma config FCMEN = OFF
// Two-speed startup disabled (Internal/External Oscillator Switchover)
#pragma config IESO = OFF

// Disable extended instruction set (not supported by XC compiler)
#pragma config XINST = OFF

// FOSC = HS, no pll, INTRC disabled
#pragma config FOSC = HS, FOSC2 = ON

// Debugger
#pragma config DEBUG = ON
    
// No code protection
#pragma config CP0 = OFF

// The pointer is pointing to ROM space that will not be reset
// otherwise after the RESET the variable content can be lost.
static persistent char g_exception[3];

// Check RCON and STKPTR register for anormal reset cause
void sys_storeResetReason()
{
    // Disable all A/D channels
    ADCON1 |= 0xF;
    
    if (!RCONbits.NOT_RI)
    {
        // Software exception. 
        // Obtain last reason from appio.h 
        g_resetReason = RESET_EXC;
        g_lastException = *(const char**)(&g_exception[0]);
        g_exception[0] = g_exception[1] = g_exception[2] = 0;
        RCONbits.NOT_RI = 1;
    }
    else if (!RCONbits.NOT_POR)
    {
        // Normal Power-on startup. Ok.
        g_resetReason = RESET_POWER;
        RCONbits.NOT_POR = 1;
        RCONbits.NOT_BOR = 1;
    }    
    else if (!RCONbits.NOT_BOR)
    {
        // Brown-out reset. Low voltage.
        g_resetReason = RESET_BROWNOUT;
        RCONbits.NOT_POR = 1;
        RCONbits.NOT_BOR = 1;
    }    
    else if (!RCONbits.NOT_TO)
    {
        // Watchdog reset. Loop detected.
        g_resetReason = RESET_WATCHDOG;
        RCONbits.NOT_TO = 1;
    }
    else if (STKPTRbits.STKFUL || STKPTRbits.STKUNF)
    {
        // Stack underrun/overrun reset. 
        g_resetReason = RESET_STACKFAIL;
        STKPTRbits.STKFUL = 0;
        STKPTRbits.STKUNF = 0;
    }
    else
    {
        // Else it was reset manually (MCLR)
        g_resetReason = RESET_MCLR;
    }
    CLRWDT();
}

// Long (callable) version of fatal
void fatal(const char* str)
{
    *((const char**)(&g_exception[0])) = str;
    wait30ms();
    RESET(); // generates RCON.RI
}

void enableInterrupts()
{
    // Disable low/high interrupt mode
    RCONbits.IPEN = 0;		
    INTCONbits.GIE = 1;
    INTCONbits.PEIE = 1;
    INTCON2bits.TMR0IP = 0;		// TMR0 Low priority
    
#ifdef HAS_RS485
    // Enable low priority interrupt on transmit
    RS485_INIT_INT();
#endif
}

// The oscillator works at 25Mhz without PLL, so 1 cycle is 160nS 
void wait2ms(void)
{
	// 2ms = ~12500 * 160ns
	Delay1KTCYx(13);
}

// The oscillator works at 25Mhz without PLL, so 1 cycle is 160nS 
void wait30ms(void)
{
	// 30ms = ~187500 * 160ns
	Delay1KTCYx(188);
}

void wait1s(void)
{
    for (int i = 0; i < 33; i++)
    {
        wait30ms();
        CLRWDT();
    }
}

// The oscillator works at 25Mhz without PLL, so 1 cycle is 160nS 
void wait40us(void)
{	
    // 40us = ~256 * 160ns
    // So wait 256 cycles
    Delay10TCYx(26);
}

void wait100us(void)
{
    wait40us();
    wait40us();
    wait40us();
}

void interrupt PRIO_TYPE low_isr()
{
    // Update tick timers at ~Khz freq
    TickUpdate();
#ifdef HAS_RS485
    rs485_interrupt();
#endif
}

static int headerPtr = 0;
static WORD size = 0;
static char str[16];

static void discard(int err) {
    headerPtr = 0;
    sprintf(str, "err %d", err);
    printlnUp(str);
}

static BYTE toUpper(BYTE b) {
    if (b >= 'a' && b <= 'z') {
        return b - ('a' - 'A');
    } else {
        return b;
    }
}

void main() {
    sys_storeResetReason();
    
    appio_init();
    // Init Ticks on timer0 (low prio) module
    timers_init();
    
    RS485_TRIS_EN = 0;
    RS485_PORT_EN = 0;

    wait1s();
    
    char title[20];
    sprintf(title, "Serial Repeater ");
    char* point = title + strlen(title) - 1;
    printlnUp(title);
    sprintf(str, "%d baud ", RS485_BAUD);
    println(str);
    
    rs485_init();
    enableInterrupts();
    
    TICK_TYPE time = TickGet();
    BYTE b;
    BYTE data[16];
    //BOOL parity = FALSE;
    
    headerPtr = 0;
    
    while (1) {
        TICK_TYPE now = TickGet();
        if (now - time > (TICKS_PER_SECOND * 1.5)) {
            *point = *point ^ (' ' ^ '.'); 
            printlnUp(title);
            time = now;
        }
        CLRWDT();
        rs485_poll();
        
        if (rs485_state != RS485_LINE_RX) {
            // TX mode...
            continue;
        }

        // Rx mode. Data?
        if (rs485_readAvail() == 0) {
            continue;
        }
        rs485_read(&b, 1);

        switch (headerPtr) {
        case 0:
            if (b == 0x55) {
                headerPtr++;
            } else {
                discard(1);
            }
            break;
        case 1:
            if (b == 0xAA) {
                headerPtr++;
            } else {
                discard(2);
            }
            break;
        case 2:
            ((BYTE*)&size)[0] = b;
            headerPtr++;
            break;
        case 3:
            ((BYTE*)&size)[1] = b;
            headerPtr++;
            break;
        default:
            if (rs485_lastRc9) {
                b = toUpper(b);
            }
            data[headerPtr - 4] = b;
            headerPtr++;
            if (headerPtr - 4 >= size) {
                sprintf(str, "pack s:%d", size);
                printlnUp(str);
                sprintf(str, "%02x %02x", data[0], data[1]);
                println(str);

                // Send data back
                rs485_write(0, data, size);
                rs485_over = 1;
                headerPtr = 0;
            }
            break;
        }
    }
}
