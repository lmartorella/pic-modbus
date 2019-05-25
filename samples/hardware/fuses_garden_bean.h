#ifndef _FUSES_INCLUDE_
#define _FUSES_INCLUDE_

// Set via OSCCON (see garden/timers)
#define SYSTEM_CLOCK 4000000ul
#define _XTAL_FREQ SYSTEM_CLOCK
#define PRIO_TYPE

#undef HAS_CM1602
#undef HAS_VS1011
#undef HAS_SPI
#undef HAS_SPI_RAM
#undef HAS_IP
#undef HAS_IO
#undef HAS_DHT11

#undef DEBUGMODE

#define HAS_BUS

#undef HAS_MAX232_SOFTWARE
#define RS232_RX_TRIS TRISBbits.TRISB3
#define RS232_TX_TRIS TRISBbits.TRISB4
#define RS232_RX_PORT PORTBbits.RB3
#define RS232_TX_PORT PORTBbits.RB4
// Timer for SW RS232: TMR2
// Timer on, 1:1 prescaler and postscaler
#define RS232_TCON T2CON
#define RS232_TCON_ON 0x04
#define RS232_TCON_OFF 0x00
#define RS232_TCON_REG PR2
#define RS232_TCON_IF PIR1bits.TMR2IF
#define RS232_TCON_ACC TMR2
#define RS232_BAUD 9600
#define RS232_TCON_VALUE ((SYSTEM_CLOCK/4) / RS232_BAUD)   // 104
#define RS232_TCON_VALUE_HALF ((SYSTEM_CLOCK/4) / RS232_BAUD / 2 - 38)  // 52-38. 38 Here is the result of checking with oscilloscope the exact poll point

// ******
// RS485: use USART on 16F877 (PORTB)
// ******
#define HAS_RS485
#define RS485_BUF_SIZE 32
#define RS485_RCSTA RCSTAbits
#define RS485_TXSTA TXSTAbits
#define RS485_TXREG TXREG
#define RS485_RCREG RCREG
#define RS485_PIR_TXIF PIR1bits.TXIF
#define RS485_PIR_RCIF PIR1bits.RCIF
#define RS485_PIE_TXIE PIE1bits.TXIE
#define RS485_PIE_RCIE PIE1bits.RCIE
#define RS485_TRIS_TX TRISCbits.TRISC6
#define RS485_TRIS_RX TRISCbits.TRISC7
#define RS485_TRIS_EN TRISDbits.TRISD1
#define RS485_PORT_EN PORTDbits.RD1
#define RS485_BAUD 19200
    // For 9600:
//#define RS485_INIT_BAUD() \
//    TXSTAbits.BRGH = 1;\
//    SPBRG = 25
#define RS485_INIT_BAUD() \
     TXSTAbits.BRGH = 1;\
     BAUDCTLbits.BRG16 = 0;\
     SPBRGH = 0;\
     SPBRG = 12

// *****
// Tick timer source. Uses TMR0 (8-bit prescales to 1:256), that resolve from 0.25ms to 16.7secs
// *****
#define TICK_TMR TMR0
#define TICK_TCON OPTION_REG

// Timer on, internal timer, 1:256 prescalar
// (!T0CS | !PSA , PS2:PS0)
#define TICK_TCON_1DATA (0x07)
#define TICK_TCON_0DATA (0x28)

#define TICK_INTCON_IF INTCONbits.T0IF
#define TICK_INTCON_IE INTCONbits.T0IE
#define TICK_CLOCK_BASE (SYSTEM_CLOCK / 4)
#define TICK_PRESCALER 256
#define TICK_TYPE WORD

// The protocol led is overridden with the first button (in OFF state)
#define HAS_LED
#define LED_PORTBIT PORTAbits.RA3
#define LED_TRISBIT TRISAbits.TRISA3


// Reset the device with fatal error
extern persistent BYTE g_exceptionPtr;
#define fatal(msg) { g_exceptionPtr = (BYTE)msg; RESET(); }

#define HAS_CUSTOM_SINK
#define SINK_CUSTOM_ID "GARD"
#define customsink_read gardenSink_read
#define customsink_write gardenSink_write

extern bit gardenSink_read();
extern bit gardenSink_write();

#endif




