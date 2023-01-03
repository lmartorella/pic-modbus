#include "net/net.h"

#include <stdlib.h>
#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <errno.h> 
#include <math.h>

/**
 * Uses the PL011 UART that supports 9-bit transmission.
 * Access the feature at low-level via memory mapped I/O.
 * TODO: implement a interrupt-based version of this UART polling. Required interrupt to be
 * routed on Raspbian.
 */

typedef enum {
    UART_REG_DR = 0x00,
    UART_REG_RSRECR = 0x01,
    UART_REG_FR = 0x06,
    UART_REG_IBRD = 0x09,
    UART_REG_FBRD = 0x0a,
    UART_REG_LCRH = 0x0b,
    UART_REG_CR = 0x0c,
    //UART_REG_IFLS = 0x0d,
    UART_REG_IMSC = 0x0e,
    //UART_REG_RIS = 0x0f,
    //UART_REG_MIS = 0x10,
    UART_REG_ICR = 0x11,
    //UART_REG_DMACR = 0x12
} UART_REGS;

typedef enum {
    UART_REG_FR_TXFE = 0x80,
    UART_REG_FR_RXFF = 0x40,
    UART_REG_FR_TXFF = 0x20,
    UART_REG_FR_RXFE = 0x10,
    UART_REG_FR_BUSY = 0x08
} UART_REG_FR_BITS;

typedef enum {
    // Sticky parity
    UART_REG_LCRH_SPS = 0x80,
    UART_REG_LCRH_WLEN8 = 0x60,
    UART_REG_LCRH_FEN = 0x10,
    UART_REG_LCRH_EPS = 0x04,
    UART_REG_LCRH_PEN = 0x02,
            
    UART_REG_LCRH_SPS_1 = (UART_REG_LCRH_SPS | 0),
    UART_REG_LCRH_SPS_0 = (UART_REG_LCRH_SPS | UART_REG_LCRH_EPS)
} UART_REG_LCRH_BITS;

typedef enum {
    UART_REG_CR_RXE = 0x200,
    UART_REG_CR_TXE = 0x100,
    UART_REG_CR_UARTEN = 0x01
} UART_REG_CR_BITS;

typedef enum {
    UART_REG_DR_FE = 0x100,
    UART_REG_DR_PE = 0x200,
    UART_REG_DR_BE = 0x400,
    UART_REG_DR_OE = 0x800
} UART_REG_DR_BITS;

typedef enum {
    UART_REG_RSRECR_FE = 0x1,
    UART_REG_RSRECR_PE = 0x2,
    UART_REG_RSRECR_BE = 0x4,
    UART_REG_RSRECR_OE = 0x8
} UART_REG_RSRECR_BITS;

typedef enum {
    GPIO_MODE_INPUT = 0,
    GPIO_MODE_OUTPUT = 1
} GPIO_MODE;

typedef enum {
    GPIO_REG_GPFSEL0 = 0,
    GPIO_REG_GPSET0 = 7,
    GPIO_REG_GPCLR0 = 10
} GPIO_REGS;

typedef struct {
    void* vmap;
    uint32_t size;
    int fdMem;
    volatile uint32_t* ptr;
} Mmap;

static Mmap* mmap_create(uint32_t base, uint32_t len) {
    Mmap* ret = (Mmap*)malloc(sizeof(Mmap));
    uint32_t page_size = 0x4000;//getpagesize();
    
    // Round first uint8_t to page boundary
    uint32_t addr0 = (base / page_size) * page_size;
    uint32_t offs = base - addr0;
    // Round last uint8_t to page boundary
    uint32_t addr1 = ((base + len - 1) / page_size) * page_size;
    ret->size = (addr1 - addr0) + page_size;
 
    ret->fdMem = open("/dev/mem", O_RDWR | O_SYNC);
    if (ret->fdMem < 0) {
       fatal("Mem open");
    }
    char* mmap_vmap = mmap(NULL, ret->size, PROT_READ | PROT_WRITE | PROT_EXEC, MAP_SHARED | MAP_LOCKED, ret->fdMem, addr0);
    if (mmap_vmap == MAP_FAILED) { 
       fatal("Mem map");
    }
    ret->ptr = (uint32_t*)(mmap_vmap + offs);
    return ret;
}

static void mmap_destroy(Mmap* map) {
    if (munmap(map->vmap, map->size) != 0) {
       fatal("Mem unmap");
    }
    if (close(map->fdMem) != 0) {
       fatal("Mem close");
    }
    free(map);
}

static void mmap_wr(const Mmap* map, int regaddr, uint32_t value) {
    map->ptr[regaddr] = value;
}

static uint32_t mmap_rd(const Mmap* map, int regaddr) {
    return map->ptr[regaddr];
}

static void gpio_setMode(Mmap* map, unsigned gpio, GPIO_MODE mode)
{
   int reg = GPIO_REG_GPFSEL0 + (gpio / 10);
   int shift = (gpio % 10) * 3;
   mmap_wr(map, reg, (map->ptr[reg] & ~(7 << shift)) | (mode << shift));
}

static void gpio_write(Mmap* map, unsigned gpio, unsigned value)
{
    int bank = gpio >> 5;
    int bit = (1 << (gpio & 0x1F));
    if (!value) {
        mmap_wr(map, GPIO_REG_GPCLR0 + bank, bit);
    }
    else {
        mmap_wr(map, GPIO_REG_GPSET0 + bank, bit);
    }
}

static uint32_t ibrd, fbrd;
static Mmap* uartMap;
static Mmap* gpioMap;

static _Bool s_rc9;
static _Bool s_txEn;
static _Bool s_rxEn;

// Sometimes the PL011 gets stuck in busy mode.
static const TICK_TYPE MAX_WAIT_TICKS = TICKS_PER_SECOND / 4;

static void uart_reset() {
    // 1. Disable UART
    mmap_wr(uartMap, UART_REG_CR, 0);

    // 2. Wait for the end of transmission or reception of the current character
    TICK_TYPE start = timers_get();
    while (mmap_rd(uartMap, UART_REG_FR) & UART_REG_FR_BUSY) {
        if (timers_get() - start > MAX_WAIT_TICKS) {
            // Something wrong. De-stuck it breaking the loop
            flog("UART_REG_FR_BUSY: %s", "Stuck");
            break;
        }
    }

    // 3. Flush the transmit FIFO by setting the FEN bit to 0 in the Line Control Register, UART_LCRH
    mmap_wr(uartMap, UART_REG_LCRH, 0);
    // 4. Reprogram the Control Register, UART_LCR, writing xBRD registers and the LCRH at the end (strobe)
    mmap_wr(uartMap, UART_REG_IBRD, ibrd);
    mmap_wr(uartMap, UART_REG_FBRD, fbrd);
    mmap_wr(uartMap, UART_REG_LCRH, UART_REG_LCRH_WLEN8 | UART_REG_LCRH_FEN | (s_rc9 ? UART_REG_LCRH_SPS_1 : UART_REG_LCRH_SPS_0) | UART_REG_LCRH_PEN);
    // 5. Enable the UART.
    uint32_t en = UART_REG_CR_UARTEN;
    if (s_txEn) en = en | UART_REG_CR_TXE;
    if (s_rxEn) en = en | UART_REG_CR_RXE;
    mmap_wr(uartMap, UART_REG_CR, en);
}

void uart_init() {
    // TODO: get the peripherial frequency
    // Measure it with "vcgencmd measure_clock uart"
    double fb = 48000000.0 / (16.0 * RS485_BAUD);
    ibrd = (uint32_t)floor(fb);
    fbrd = (uint32_t)(fmod(fb, 1.0) * 64);

    s_rc9 = 0;
    s_txEn = 0;
    s_rxEn = 0;

    const uint32_t pi_peri_phys = 0x20000000;
    uartMap = mmap_create((pi_peri_phys + 0x00201000), 0x90);
    gpioMap = mmap_create((pi_peri_phys + 0x00200000), 0xB4);

    // Disable UART interrupts (avoid clash with OS)
    mmap_wr(uartMap, UART_REG_IMSC, 0);
    mmap_wr(uartMap, UART_REG_ICR, 0);
    
    // MAX485 pin as output
    gpio_setMode(gpioMap, 2, GPIO_MODE_OUTPUT);
}

// MAX485 levels
#define EN_TRANSMIT 1
#define EN_RECEIVE 0

void uart_transmit() {
    gpio_write(gpioMap, 2, EN_TRANSMIT);
}

void uart_receive() {
    gpio_write(gpioMap, 2, EN_RECEIVE);
}

void uart_set_9b(_Bool b) {
    if (b != s_rc9) {
        s_rc9 = b;
        uart_reset();
    }
}

void uart_write(uint8_t b) {
    mmap_wr(uartMap, UART_REG_DR, b);
#ifdef DEBUGMODE
    printf(" T%c%02x ", s_rc9 ? '1' : '0', b);
#endif
}

void uart_read(uint8_t* data, UART_RX_MD* md) {
    // Read data
    uint32_t rx = mmap_rd(uartMap, UART_REG_DR);
    *data = rx & 0xff;
    // And then read ERRORS associated to that uint8_t
    md->overrunErr = (rx & UART_REG_DR_OE) != 0;
    md->frameErr = (rx & UART_REG_DR_FE) != 0;

#ifdef DEBUGMODE
    printf(" R");
    if (rx & ~(0xff | UART_REG_DR_PE)) {
        printf("E");
    }
    printf("%c%02x ", md->rc9 ? '1' : '0', *data);
#endif

    // Reset errors
    mmap_wr(uartMap, UART_REG_RSRECR, 0);
}

_Bool uart_tx_fifo_empty() {
    return s_txEn && (mmap_rd(uartMap, UART_REG_FR) & UART_REG_FR_TXFE);
}

_Bool uart_rx_fifo_empty() {
    return s_rxEn && (mmap_rd(uartMap, UART_REG_FR) & UART_REG_FR_RXFE);
}

void uart_enable_tx() {
    s_txEn = true;
    uart_reset();
}

void uart_disable_tx() {
    s_txEn = false;
    uart_reset();
}

void uart_enable_rx() {
    s_rxEn = true;
    uart_reset();
}

void uart_disable_rx() {
    s_rxEn = false;
    uart_reset();
}

