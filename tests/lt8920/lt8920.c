#include <stdint.h>
#include "hw.h"

static void reset() {
    gpio_reset();
}

typedef union {
    struct {
        unsigned channel: 7;
        unsigned rx_en: 1;
        unsigned tx_en: 1;
        unsigned res: 7;
    } b;
    uint16_t v;
} REG_7;

static void set_reg(uint8_t reg, uint16_t val) {
    
}

static uint16_t get_reg(uint8_t reg) {
    
}

static void init_registers() {
    {
        REG_7 reg;
        reg.v = get_reg(7);
        reg.b.channel = LT8920_CHANNEL;
        reg.b.rx_en = 0;
        reg.b.tx_en = 0;
        set_reg(7, reg.v);
    }

    // From spreadsheet
    set_reg(9, 0x4800); // (set power to maximum) Sets Tx power level
    set_reg(10, 0x7ffd); // Crystal osc. enabled.
    set_reg(11, 0x0008); // RSSI enabled.
    set_reg(23, 0x8005); // Calibrate VCO before each and every Tx/Rx.
    set_reg(27, 0x1300); // No crystal trim.
    set_reg(32, 0x4808); // Packet data type: NRZ, no FEC, BRCLK=12 div. by 4= 3MHz
    set_reg(33, 0x3fc7); // Configures packet sequencing.
    set_reg(34, 0x2000); // Configures packet sequencing.
    set_reg(35, 0x0300); // AutoAck max Tx retries = 3

    // Choose unique sync words for each over-the-air network.
    // Similar to a MAC address.
    set_reg(36, LT8920_SYNC_WORD_0);
    set_reg(37, LT8920_SYNC_WORD_1); 
    set_reg(38, LT8920_SYNC_WORD_2);
    set_reg(39, LT8920_SYNC_WORD_3);

    set_reg(40, 0x2102); // Configure FIFO flag, sync threshold.
    set_reg(41, 0xb000); // CRC on. SCRAMBLE off. 1st byte is packet length. 
    set_reg(42, 0xfdb0); // AutoACK off.
    set_reg(43, 0x000f); // Configure scan_rssi.
    set_reg(44, 0x1000); // Configure data rate
    set_reg(45, 0x0080); // 0080h for 1Mbps, 0552h for /others 
}

/**
 * Setup connection
 */
void lt8920_init() {
    reset();
    // Wait T1 (1 to 5ms) for crystal oscillator to stabilize
    sleep(5);
    init_registers();
    // Now in read mode
}

/**
 * Returns `true` if the bus is active (so fast poll is required).
 */
_Bool lt8920_poll() {

}

/**
 * Set to true when the line is not active from more than 3.5 characters (ModBus mark condition)
 */
_Bool lt8920_isMarkCondition;

/**
 * The whole buffer. `LT8920_BUF_SIZE` should be at least 16 bytes.
 * The buffer data should not be accessed until operation is completed.
 */
uint8_t lt8920_buffer[LT8920_BUF_SIZE];

/**
 * Start writing the data in the `lt8920_buffer`.
 * `size` is the number of bytes valid in the buffer to write.
 */ 
void lt8920_write(uint8_t size);

/**
 * Discard `count` bytes from the read buffer
 */
void lt8920_discard(uint8_t count);

/**
 * Get count of available bytes in the read `lt8920_buffer`
 */
uint8_t lt8920_readAvail();

/**
 * Check if the buffer contains data still to be sent
 */
_Bool lt8920_writeInProgress();

/**
 * State of the LT8290 line
 */
typedef enum {
    // Receiving, all OK
    LT8290_LINE_RX,
    // End of transmitting, in disengage line period
    LT8290_LINE_TX_DISENGAGE,
    // Transmitting, data
    LT8290_LINE_TX,
    // After TX engaged, wait before transmitting
    LT8290_LINE_WAIT_FOR_START_TRANSMIT
} LT8290_LINE_STATE;
LT8290_LINE_STATE lt8290_state;

