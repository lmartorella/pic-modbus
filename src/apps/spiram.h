#ifndef _23K256_INCLUDE_
#define _23K256_INCLUDE_

#ifdef HAS_SPI_RAM

#define MEM_CS0	MEM_PORTBITS.MEM_BANK0_CS
#define MEM_CS1	MEM_PORTBITS.MEM_BANK1_CS
#define MEM_CS2	MEM_PORTBITS.MEM_BANK2_CS
#define MEM_CS3	MEM_PORTBITS.MEM_BANK3_CS

// This will override the spi_init() call
void sram_init(void);
// Test all 4 banks, displays the ADDR of the failing test and hang if found one
// bs is the BYTE seed
void sram_test_gui(BYTE* buffer, UINT16 bufferSize);

// Read a vector of bytes in RAM.
// NOTE: do not support SPI cross-bank access
//  - *dest is in banked PIC RAM
//  - address is logic SPIRAM address of the first byte to read
//  - count is the count of byes to read
void sram_read(BYTE* dest, UINT32 address, UINT16 count);

// Write a vector of bytes in RAM.
// NOTE: do not support SPI cross-bank access
//  - *dest is in banked PIC RAM
//  - address is logic SPIRAM address of the first byte to write
//  - count is the count of byes to write
void sram_write(const BYTE* src, UINT32 address, UINT16 count);

#endif
#endif
