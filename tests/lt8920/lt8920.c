#include <stdint.h>
#include "lt8920.h"
#include "hw.h"

#define get_reg_8 spi_get_reg8
#define set_reg_8 spi_set_reg8
#define get_reg spi_get_reg16_msb_first
#define set_reg spi_set_reg16_msb_first

LT8920_REGISTER_CACHE lt8920_registers;

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
    _debug_print_init_reg(reg, cur_val, new_val);
    return new_val;
}

static void lt8920_init_registers() {
    // Stop TX/RX packets. Select the channel
    lt8920_registers.reg7.b.channel = LT8920_CHANNEL;
    lt8920_registers.reg7.b.rx_en = 0;
    lt8920_registers.reg7.b.tx_en = 0;
    lt8920_registers.reg7.v = init_reg(7, lt8920_registers.reg7.v, REG_7_MASK);

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

    lt8920_registers.fifo_ctrl.v;
    lt8920_registers.fifo_ctrl.b.CLR_R_PTR = 1;
    lt8920_registers.fifo_ctrl.v = init_reg(R_FIFO_CONTROL, lt8920_registers.fifo_ctrl.v, REG_FIFO_CONTROL_MASK);
}

void lt8920_reset() {
    gpio_reset(true);
    usleep(5000);
    gpio_reset(false);
    // Wait T1 (1 to 5ms) for crystal oscillator to stabilize
    usleep(5000);
    // Init regs
    lt8920_init_registers();
}

/**
 * Disable RX/TX
*/
void lt8920_disable_rx_tx() {
    // turn off rx/tx
    lt8920_registers.reg7.b.rx_en = 0;
    lt8920_registers.reg7.b.tx_en = 0;
    set_reg(7, lt8920_registers.reg7.v);
    usleep(3000);
}

void lt8920_flush_rx() {
    // flush rx
    lt8920_registers.fifo_ctrl.b.CLR_R_PTR = 1;
    set_reg(R_FIFO_CONTROL, lt8920_registers.fifo_ctrl.v);
    lt8920_registers.fifo_ctrl.b.CLR_R_PTR = 0;
}

void lt8920_flush_tx() {
    // flush rx
    lt8920_registers.fifo_ctrl.b.CLR_W_PTR = 1;
    set_reg(R_FIFO_CONTROL, lt8920_registers.fifo_ctrl.v);
    lt8920_registers.fifo_ctrl.b.CLR_W_PTR = 0;
}

void lt8920_enable_rx() {
    // Start read
    lt8920_registers.reg7.b.rx_en = 1;
    set_reg(7, lt8920_registers.reg7.v);
}

void lt8920_enable_tx() {
    // Start write
    lt8920_registers.reg7.b.tx_en = 1;
    set_reg(7, lt8920_registers.reg7.v);
}

uint8_t lt8920_read_fifo() {
    return get_reg_8(R_FIFO);
}

void lt8920_write_fifo(uint8_t data) {
    set_reg_8(R_FIFO, data);
}

void lt8920_get_status() {
    lt8920_registers.status.v = get_reg(R_STATUS);
}

void lt8920_get_rev(LT8920_REVISION_INFO* info) {
    info->reg29.v = get_reg(29);
    info->reg30.v = get_reg(30);
    info->reg31.v = get_reg(31);
}
