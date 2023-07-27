#include <stdint.h>
#include "hw.h"

/**
 * State of the LT8290 line
 */
typedef enum {
    // Receiving, all OK
    LT8290_LINE_RX,
    // Transmitting, data
    LT8290_LINE_TX
} LT8290_LINE_STATE;
static LT8290_LINE_STATE lt8290_state;

static void reset() {
    gpio_reset(true);
    sleep(5);
    gpio_reset(false);
    // Wait T1 (1 to 5ms) for crystal oscillator to stabilize
    sleep(5);
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
#define REG_7_MASK ((uint16_t)~0x01ff)

#define R_FIFO (50) 

#define R_FIFO_CONTROL (52)
typedef union {
    struct {
        unsigned FIFO_RD_PTR: 5;
        unsigned res1: 1;
        unsigned CLR_R_PTR: 1;
        unsigned FIFO_WR_PTR: 5;
        unsigned res2: 1;
        unsigned CLR_W_PTR: 1;
    } b;
    uint16_t v;
} REG_FIFO_CONTROL;
#define REG_FIFO_CONTROL_MASK ((uint16_t)~0xbfbf)

#define set_reg spi_set_reg_msb_first
#define get_reg spi_get_reg_msb_first

/**
 * Init a LT8920 register, using the passed `val` but using the unmasked bits (reserved) from the 
 * current value, using a `get_reg`
 */
static uint16_t init_reg(uint8_t reg, uint16_t val, uint16_t mask) {
#ifdef UNIT_TEST
    check_init_reg(reg, val, mask);
#endif
    uint16_t cur_val = get_reg(reg);
    uint16_t new_val = (cur_val & mask) | val;
    set_reg(reg, new_val);
    return new_val;
}

static void init_registers() {
    // Stop TX/RX packets. Select the channel
    REG_7 reg7 = { 
        .b = {
            .channel = LT8920_CHANNEL,
            .rx_en = 0,
            .tx_en = 0
        }
    };
    init_reg(7, reg7.v, REG_7_MASK);

    // From LT8920 spreadsheet, with slight modifications to something likely to be issues
    init_reg(9, 0x4000, (uint16_t)~0xf780); // (set power to maximum) Sets Tx power level
    init_reg(10, 0x0001, (uint16_t)~0x0001); // Crystal osc. enabled.
    init_reg(11, 0x0000, (uint16_t)~0x0100); // RSSI enabled.
    init_reg(23, 0x0004, (uint16_t)~0x0004); // Calibrate VCO before each and every Tx/Rx.
    init_reg(27, 0x0000, (uint16_t)~0x003f); // No crystal trim.
    init_reg(32, 0x5808, (uint16_t)~0xfffe); // Packet data type: NRZ, no FEC, BRCLK=12 div. by 8= 1.5MHz, preamble = 3bytes, sync word = 64bits, trailer = 4bit
    init_reg(33, 0x3fc7, (uint16_t)~0xffff); // Configures packet sequencing, VCO_ON_DELAY_CNT = 63uS, TX_PA_OFF_DELAY = 4us + 3us, TX_PA_ON_DELAY = 7us
    init_reg(34, 0x200b, (uint16_t)~0xff3f); // Configures packet sequencing. Bpktctl_direct = 0, TX_CW_DLY = 32, TX_SW_ON_DELAY = 11us
    init_reg(35, 0x0300, (uint16_t)~0xdfff); // AutoAck max Tx retries = 3, POWER_DOWN = 0, SLEEP_MODE = 0, BRCLK_ON_SLEEP = 0, RETRANSMIT_TIMES = 3, MISO_TRI_OPT = 0, SCRAMBLE_DATA = 0

    // Choose unique sync words for each over-the-air network.
    // Similar to a MAC address.
    set_reg(36, LT8920_SYNC_WORD_0);
    set_reg(37, LT8920_SYNC_WORD_1); 
    set_reg(38, LT8920_SYNC_WORD_2);
    set_reg(39, LT8920_SYNC_WORD_3);

    init_reg(40, 0x2102, (uint16_t)~0xffff); // Configure FIFO flag, FIFO_EMPTY_THRESHOLD = 8, FIFO_FULL_THRESHOLD = 8, SYNCWORD_THRESHOLD = 2
    init_reg(41, 0xb000, (uint16_t)~0xfcff); // CRC on. SCRAMBLE off. 1st byte is packet length, FW_TERM_TX = 0, AUTO_ACK = 0, PKT_FIFO_POLARITY = 0, CRC_INITIAL_DATA = 0
    init_reg(42, 0xfcb0, (uint16_t)~0xfcff); // SCAN_RSSI_CH_NO = 63, Rx_ACK_TIME[7:0] = 176us
    init_reg(43, 0x000f, (uint16_t)~0xffff); // SCAN_RSSI_EN = 0, SCAN_STRT_CH_OFFST[6:0] = 0, WAIT_RSSI_SCAN_TIM[7:0] = 15us
    init_reg(44, 0x1000, (uint16_t)~0xff00); // DATARATE[7:0] = 0x10 = 62.5Kbps
    init_reg(45, 0x0552, (uint16_t)~0xffff); // OPTION: 0080h for 1Mbps, 0552h for /others
}

/**
 * Setup connection
 */
void lt8920_init() {
    reset();
    init_registers();
    // Now in read mode
}

/**
 * Returns `true` if the bus is active (so fast poll is required).
 */
_Bool lt8920_poll() {
    return false;
}

static void lt8920_startRead() {
    // turn off rx/tx
    REG_7 reg7 = { 
        .b = {
            .channel = LT8920_CHANNEL,
            .rx_en = 0,
            .tx_en = 0
        }
    };
    reg7.v = init_reg(7, reg7.v, REG_7_MASK);
    sleep(3);

    // flush rx
    REG_FIFO_CONTROL reg_fifo_ctrl = {
        .v = 0
    };
    reg_fifo_ctrl.b.CLR_R_PTR = 1;
    reg_fifo_ctrl.v = init_reg(R_FIFO_CONTROL, reg_fifo_ctrl.v, REG_FIFO_CONTROL_MASK);

    reg7.b.rx_en = 1;
    set_reg(7, reg7.v);

    lt8290_state = LT8290_LINE_RX;
}

/**
 * The whole buffer. `LT8920_BUF_SIZE` should be at least 16 bytes.
 * The buffer data should not be accessed until operation is completed.
 */
uint8_t lt8920_buffer[LT8920_BUF_SIZE];

/**
 * Start writing the data in the `lt8920_buffer`.
 * `size` is the number of bytes valid in the buffer to write.
 */ 
void lt8920_write_packet(uint8_t size) {
    // turn off rx/tx
    REG_7 reg7 = { 
        .b = {
            .channel = LT8920_CHANNEL,
            .rx_en = 0,
            .tx_en = 0
        }
    };
    reg7.v = init_reg(7, reg7.v, REG_7_MASK);
    sleep(3);

    // flush tx
    REG_FIFO_CONTROL reg_fifo_ctrl = {
        .v = 0
    };
    reg_fifo_ctrl.b.CLR_W_PTR = 1;
    reg_fifo_ctrl.v = init_reg(R_FIFO_CONTROL, reg_fifo_ctrl.v, REG_FIFO_CONTROL_MASK);

    uint8_t pos = 0;
    set_reg(R_FIFO, (size << 8) | lt8920_buffer[pos++]);
    while (pos < size) {
        uint8_t msb = lt8920_buffer[pos++];
        // Last one can contains garbage
        uint8_t lsb = lt8920_buffer[pos++];
        set_reg(R_FIFO, (msb << 8) | lsb);
    }

    // Enable TX
    reg7.b.tx_en = 1;
    set_reg(7, reg7.v);

    lt8290_state = LT8290_LINE_TX;
}

/**
 * Discard `count` bytes from the read buffer
 */
void lt8920_discard(uint8_t count) {

}

/**
 * Get count of available bytes in the read `lt8920_buffer`
 */
uint8_t lt8920_readAvail() {
    return -1;
}

/**
 * Check if the buffer contains data still to be sent
 */
_Bool lt8920_writeInProgress() {
    return false;
}

