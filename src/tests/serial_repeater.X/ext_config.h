#ifndef EXT_CONFIG_H
#define	EXT_CONFIG_H

#define SYSTEM_CLOCK 25000000ull
extern void wait30ms();
extern void wait40us();
extern void wait2ms();
extern void wait100us();
// Reset the device with fatal error
void fatal(const char* msg);
#define PRIO_TYPE low_priority

// *****
// Tick timer source
// *****
#define TICK_TMRH TMR0H
#define TICK_TMRL TMR0L
#define TICK_TCON T0CON
// Timer on, 16-bit, internal timer, 1:256 prescalar
// (TMR0ON | T0PS2 | T0PS1 | T0PS0)
#define TICK_TCON_DATA (0x87)
#define TICK_INTCON_IF INTCONbits.TMR0IF
#define TICK_INTCON_IE INTCONbits.TMR0IE
#define TICK_CLOCK_BASE (SYSTEM_CLOCK/4ull)
#define TICK_PRESCALER 256ull

#define TICK_TYPE DWORD
#define _IS_ETH_CARD

// ******* 
// DISPLAY
// Uses PORTE, 0, 2-7
// ******* 
#define HAS_CM1602
#define CM1602_PORT 		PORTE
#define CM1602_TRIS 		TRISE
#define CM1602_PORTADDR         0xF84
#define CM1602_IF_MODE 		4
#define CM1602_IF_NIBBLE_LOW 	0
#define CM1602_IF_NIBBLE_HIGH 	1
#define CM1602_IF_NIBBLE 	CM1602_IF_NIBBLE_HIGH
#define CM1602_IF_TRIS_RW 	TRISEbits.RE2
#define CM1602_IF_TRIS_RS 	TRISEbits.RE0
#define CM1602_IF_TRIS_EN 	TRISEbits.RE3
#define CM1602_IF_BIT_RW 	PORTEbits.RE2
#define CM1602_IF_BIT_RS 	PORTEbits.RE0
#define CM1602_IF_BIT_EN 	PORTEbits.RE3
#define CM1602_LINE_COUNT 	2
#define CM1602_COL_COUNT 	16
#define CM1602_FONT_HEIGHT 	7

// ******
// RS485: use USART1 on 18F87J60 (PORTG)
// ******
#define HAS_RS485
#define RS485_BUF_SIZE 64
#define RS485_RCSTA RCSTA2bits
#define RS485_TXSTA TXSTA2bits
#define RS485_TXREG TXREG2
#define RS485_RCREG RCREG2
#define RS485_PIR_TXIF PIR3bits.TX2IF
#define RS485_PIR_RCIF PIR3bits.RC2IF
#define RS485_PIE_TXIE PIE3bits.TX2IE
#define RS485_PIE_RCIE PIE3bits.RC2IE
#define RS485_TRIS_TX TRISGbits.RG1
#define RS485_TRIS_RX TRISGbits.RG2
#define RS485_TRIS_EN TRISGbits.RG0
#define RS485_PORT_EN PORTGbits.RG0
#define RS485_BAUD 19200
    // For 9600:
//#define RS485_INIT_BAUD() \
//    TXSTA1bits.BRGH = 1;\
//    BAUDCON1bits.BRG16 = 0;\
//    SPBRGH1 = 0;\
//    SPBRG1 = 162
 #define RS485_INIT_BAUD() \
     TXSTA2bits.BRGH = 1;\
     BAUDCON2bits.BRG16 = 0;\
     SPBRGH2 = 0;\
     SPBRG2 = 80
#define RS485_INIT_INT() \
    IPR3bits.TX2IP = 0; \
    IPR3bits.RC2IP = 0


#endif	/* CIUCCIA_H */

