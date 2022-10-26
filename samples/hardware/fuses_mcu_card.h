#ifndef _FUSES_INCLUDE_
#define _FUSES_INCLUDE_

// ==== MCU CARD 1.0

// PORTA: 0/5, Digital and analog. RA0/RA1 used by ethernet leds
// PORTB: 0/7, interrupt on change. RB6/7 used by ICSP
// PORTC: 4 +6/7(USART1) used by MAX485 + enable logic
// PORTG: 4: bus power switch
// PORTH: 0/7 : Used by CM1602 module + LED

// ==== MCU CARD Prototype boards

// PORTA: 0/5, Digital and analog. RA0/RA1 used by ethernet leds
// PORTB: 0/7, interrupt on change. RB6/7 used by ICSP. RB0/5 used by Mp3
// PORTC: RC3/4 used by I2C, 1/2 and 6/7 used by EXTRAM I2C.
// PORTD: 0/2
// PORTE: 0/5: Used by CM1602 module (0 and 2/7)
// PORTG: 4: 1/2 used by USART2 + 0 used by MAX485 enable logic

#define SYSTEM_CLOCK 25000000ull
#define _XTAL_FREQ (SYSTEM_CLOCK)
#define PRIO_TYPE low_priority

#define HAS_RS485_BUS_PRIMARY
#define MASTER_MAX_CHILDREN 4

// ******* 
// DISPLAY
// ******* 
#define HAS_CM1602
#ifdef PROTO_PINOUT
    #define CM1602_PORT 		PORTE
    #define CM1602_TRIS 		TRISE
    #define CM1602_PORTADDR     0xF84
    #define CM1602_IF_TRIS_RW 	TRISEbits.RE2
    #define CM1602_IF_TRIS_RS 	TRISEbits.RE0
    #define CM1602_IF_TRIS_EN 	TRISEbits.RE3
    #define CM1602_IF_BIT_RW 	PORTEbits.RE2
    #define CM1602_IF_BIT_RS 	PORTEbits.RE0
    #define CM1602_IF_BIT_EN 	PORTEbits.RE3
#else
    #define CM1602_PORT 		PORTH
    #define CM1602_TRIS 		TRISH
    #define CM1602_PORTADDR     0xF87
    #define CM1602_IF_TRIS_RW 	TRISHbits.RH1
    #define CM1602_IF_TRIS_RS 	TRISHbits.RH3
    #define CM1602_IF_TRIS_EN 	TRISHbits.RH0
    #define CM1602_IF_BIT_RW 	PORTHbits.RH1
    #define CM1602_IF_BIT_RS 	PORTHbits.RH3
    #define CM1602_IF_BIT_EN 	PORTHbits.RH0
#endif
#define CM1602_IF_NIBBLE 	CM1602_IF_NIBBLE_HIGH
#define CM1602_IF_MODE 		4
#define CM1602_IF_NIBBLE_LOW 	0
#define CM1602_IF_NIBBLE_HIGH 	1
#define CM1602_LINE_COUNT 	2
#define CM1602_COL_COUNT 	20
#define CM1602_FONT_HEIGHT 	7

// ******* 
// MP3
// ******* 
#undef HAS_VS1011

// ******* 
// MEM & LOADER & FLASH
// *******
//#define MAX_PROG_MEM		0x20000
//#define ROM_BLOCK_SIZE		64
//#define CONFIGURATION_SIZE 	8

// ******
// SPI
// ******
#undef HAS_SPI

// ******
// SPI RAM
// ******
#undef HAS_SPI_RAM

// ******
// IP: uses PORTA0,1 (leds)
// ******

#ifdef PROTO_PINOUT
#define SERVER_CONTROL_UDP_PORT 17008
#else
#define SERVER_CONTROL_UDP_PORT 17007
#endif
#define CLIENT_TCP_PORT 20000

// ******
// IO: power line on/off
// ******
#undef HAS_DIGIO
#ifdef PROTO_PINOUT
    #define HAS_LED
    #define LED_PORTBIT PORTHbits.RH0
    #define LED_TRISBIT TRISHbits.RH0
#else
    #define BUSPOWER_TRIS TRISGbits.RG4
    #define BUSPOWER_PORT PORTGbits.RG4
#endif

// ******
// RS485: use USART1 on 18F87J60 (PORTG)
// ******
#define RS485_BUF_SIZE 64
#ifdef PROTO_PINOUT
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
    //    TXSTA2bits.BRGH = 1;\
    //    BAUDCON2bits.BRG16 = 0;\
    //    SPBRGH2 = 0;\
    //    SPBRG2 = 162
    #define RS485_INIT_BAUD() \
         TXSTA2bits.BRGH = 1;\
         BAUDCON2bits.BRG16 = 0;\
         SPBRGH2 = 0;\
         SPBRG2 = 80
    #define RS485_INIT_INT() \
        IPR3bits.TX2IP = 0; \
        IPR3bits.RC2IP = 0
#else
    #define RS485_RCSTA RCSTA1bits
    #define RS485_TXSTA TXSTA1bits
    #define RS485_TXREG TXREG1
    #define RS485_RCREG RCREG1
    #define RS485_PIR_TXIF PIR1bits.TX1IF
    #define RS485_PIR_RCIF PIR1bits.RC1IF
    #define RS485_PIE_TXIE PIE1bits.TX1IE
    #define RS485_PIE_RCIE PIE1bits.RC1IE
    #define RS485_TRIS_TX TRISCbits.RC6
    #define RS485_TRIS_RX TRISCbits.RC7
    #define RS485_TRIS_EN TRISCbits.RC4
    #define RS485_PORT_EN PORTCbits.RC4
    #define RS485_BAUD 19200
        // For 9600:
    //#define RS485_INIT_BAUD() \
    //    TXSTA1bits.BRGH = 1;\
    //    BAUDCON1bits.BRG16 = 0;\
    //    SPBRGH1 = 0;\
    //    SPBRG1 = 162
     #define RS485_INIT_BAUD() \
         TXSTA1bits.BRGH = 1;\
         BAUDCON1bits.BRG16 = 0;\
         SPBRGH1 = 0;\
         SPBRG1 = 80
    #define RS485_INIT_INT() \
        IPR1bits.TX1IP = 0; \
        IPR1bits.RC1IP = 0
#endif
    
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

#define TICK_TYPE uint32_t

#ifdef HAS_IO
#ifdef HAS_SPI
#error Cannot use SPI and IO togheter
#elif defined(HAS_SPI_RAM)
#error Cannot use SPI RAM and IO togheter
#endif
#endif

// persistent char* are not supported by xc8 1.37
#define LAST_EXC_TYPE long
// Reset the device with fatal error
void fatal(const char* msg);

#endif

