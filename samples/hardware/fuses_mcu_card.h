#ifndef _FUSES_INCLUDE_
#define _FUSES_INCLUDE_

// ==== MCU CARD 1.0

// PORTA: 0/5, Digital and analog. RA0/RA1 used by ethernet leds
// PORTB: 0/7, interrupt on change. RB6/7 used by ICSP
// PORTC: 4 +6/7(USART1) used by MAX485 + enable logic
// PORTG: 4: bus power switch
// PORTH: 0/7 : Used by CM1602 module + LED

#define SYSTEM_CLOCK 25000000ull
#define PRIO_TYPE low_priority

#define HAS_BUS

// ******* 
// DISPLAY
// Uses PORTH
// ******* 
#define HAS_CM1602
#define CM1602_PORT 		PORTH
#define CM1602_TRIS 		TRISH
#define CM1602_PORTADDR     0xF87
#define CM1602_IF_MODE 		4
#define CM1602_IF_NIBBLE_LOW 	0
#define CM1602_IF_NIBBLE_HIGH 	1
#define CM1602_IF_NIBBLE 	CM1602_IF_NIBBLE_HIGH
#define CM1602_IF_TRIS_RW 	TRISHbits.RH1
#define CM1602_IF_TRIS_RS 	TRISHbits.RH3
#define CM1602_IF_TRIS_EN 	TRISHbits.RH0
#define CM1602_IF_BIT_RW 	PORTHbits.RH1
#define CM1602_IF_BIT_RS 	PORTHbits.RH3
#define CM1602_IF_BIT_EN 	PORTHbits.RH0
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
#define MAX_PROG_MEM		0x20000
#define ROM_BLOCK_SIZE		64
#define CONFIGURATION_SIZE 	8

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

// 17007: release, 17008: debug
#define SERVER_CONTROL_UDP_PORT 17007
// 20000: release, 20001: debug
#define CLIENT_TCP_PORT 20000

// ******
// IO: power line on/off
// ******
//#undef HAS_DIGIO
//#define DIGIO_TRIS_IN_BIT TRISGbits.RG4
//#define DIGIO_PORT_IN_BIT PORTGbits.RG4
//#define DIGIO_TRIS_OUT_BIT TRISGbits.RG4
//#define DIGIO_PORT_OUT_BIT PORTGbits.RG4
#define BUSPOWER_TRIS TRISGbits.RG4
#define BUSPOWER_PORT PORTGbits.RG4

// ******
// RS485: use USART1 on 18F87J60 (PORTG)
// ******
#define HAS_RS485
#define RS485_BUF_SIZE 64
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

#ifdef HAS_IO
#ifdef HAS_SPI
#error Cannot use SPI and IO togheter
#elif defined(HAS_SPI_RAM)
#error Cannot use SPI RAM and IO togheter
#endif
#endif

// Reset the device with fatal error
void fatal(const char* msg);

#endif

