#ifndef FUSES_MICRO_BEAN_H
#define	FUSES_MICRO_BEAN_H

#define SYSTEM_CLOCK 4000000ul
#define _XTAL_FREQ SYSTEM_CLOCK

// Internal core clock drives timer with 1:256 prescaler
typedef uint16_t TICK_TYPE; 
#define TICKS_PER_SECOND		(TICK_TYPE)((TICK_CLOCK_BASE + (TICK_PRESCALER / 2ull)) / TICK_PRESCALER)	
#define TICKS_PER_MILLISECOND		(TICK_TYPE)(TICKS_PER_SECOND / 1000)

#define PRIO_TYPE
#define EEPROM_MODIFIER __eeprom

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

#define HAS_LED
#define LED_PORTBIT PORTAbits.RA7
#define LED_TRISBIT TRISAbits.TRISA7

// ******
// RS485: use USART1 on 16F628 (PORTB)
// ******
#define RS485_BUF_SIZE 32
#define RS485_RCSTA RCSTAbits
#define RS485_TXSTA TXSTAbits
#define RS485_TXREG TXREG
#define RS485_RCREG RCREG
#define RS485_PIR_TXIF PIR1bits.TXIF
#define RS485_PIR_RCIF PIR1bits.RCIF
#define RS485_PIE_TXIE PIE1bits.TXIE
#define RS485_PIE_RCIE PIE1bits.RCIE
#define RS485_TRIS_TX TRISBbits.TRISB5
#define RS485_TRIS_RX TRISBbits.TRISB2
#define RS485_TRIS_EN TRISAbits.TRISA6
#define RS485_PORT_EN PORTAbits.RA6
#if RS485_BAUD == 9600
#define RS485_INIT_BAUD() \
     TXSTAbits.BRGH = 1;\
     SPBRG = 25;\
     BAUDCONbits.SCKP = 0;\
     BAUDCONbits.BRG16 = 0;\
     APFCON0bits.RXDTSEL = 1;\
     APFCON1bits.TXCKSEL = 1;
#elif RS485_BAUD == 19200
#define RS485_INIT_BAUD() \
     TXSTAbits.BRGH = 1;\
     SPBRG = 12;\
     BAUDCONbits.SCKP = 0;\
     BAUDCONbits.BRG16 = 0;\
     APFCON0bits.RXDTSEL = 1;\
     APFCON1bits.TXCKSEL = 1;
#else
    #error Baud rate not supported by macro
#endif

//RXDTSEL:1  RX/DT function is on RB2
//TXCKSEL:1  TX/CK function is on RB5

// persistent char* are not supported by xc8 1.37
typedef uint16_t EXC_STRING_T; 
#define EXC_STRING_NULL (0)

extern __persistent EXC_STRING_T g_exceptionPtr;
// Reset the device with fatal error
#define fatal(msg) { g_exceptionPtr = (EXC_STRING_T)msg; RESET(); }

#define INIT_PORTS() \
     ANSELBbits.ANSB2 = 0;\
     ANSELBbits.ANSB5 = 0;\

#endif	/* FUSES_MICRO_BEAN_H */

