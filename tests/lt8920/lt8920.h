#ifndef _LT8920_H
#define	_LT8920_H

#include "configuration.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef union {
    struct {
        unsigned channel: 7;
        unsigned rx_en: 1;
        unsigned tx_en: 1;
        unsigned _res: 7;
    } b;
    uint16_t v;
} REG_7;
#define REG_7_MASK ((uint16_t)~0x01ff)

typedef union {
    struct {
        unsigned CRC_INITIAL_DATA: 8;
        unsigned _res: 2;
        unsigned PKT_FIFO_POLARITY: 1;
        unsigned AUTO_ACK: 1;
        unsigned FW_TERM_TX: 1;
        unsigned PACK_LENGTH_EN: 1;
        unsigned SCRAMBLE_ON: 1;
        unsigned CRC_ON: 1;
    } b;
    uint16_t v;
} REG_41;
#define REG_41_MASK ((uint16_t)~0xfcff)

#define R_STATUS (48)
typedef union {
    struct {
        unsigned _res: 5;
        unsigned FIFO_FLAG: 1;
        unsigned PKT_FLAG: 1;
        unsigned SYNCWORD_RECV: 1;
        unsigned FRAMER_ST: 6;
        unsigned FEC23_ERROR: 1;
        unsigned CRC_ERROR: 1;
    } b;
    uint16_t v;
} REG_STATUS;

#define R_FIFO (50) 

#define R_FIFO_CONTROL (52)
typedef union {
    struct {
        unsigned FIFO_RD_PTR: 6;
        unsigned _res1: 1;
        unsigned CLR_R_PTR: 1;
        unsigned FIFO_WR_PTR: 6;
        unsigned _res2: 1;
        unsigned CLR_W_PTR: 1;
    } b;
    uint16_t v;
} REG_FIFO_CONTROL;
#define REG_FIFO_CONTROL_MASK ((uint16_t)~0xbfbf)

typedef struct {
    union {
        struct {
            unsigned MCU_VER_ID: 2;
            unsigned _res1: 1;
            unsigned RF_VER_ID: 4;
        } b;
        uint16_t v;
    } reg29;

    union {
        struct {
            unsigned ID_CODE_JEDEC_MCODE_L: 16;
        } b;
        uint16_t v;
    } reg30;

    union {
        struct {
            unsigned ID_CODE_JEDEC_MCODE_M: 12;
            unsigned RF_CODE_ID: 4;
        } b;
        uint16_t v;
    } reg31;
} LT8920_REVISION_INFO;

void lt8920_reset();

void lt8920_disable_rx_tx();
void lt8920_flush_rx();
void lt8920_flush_tx();
void lt8920_enable_rx();
void lt8920_enable_tx();

void lt8920_get_status();

uint8_t lt8920_read_fifo();
void lt8920_write_fifo(uint8_t data);

void lt8920_get_rev(LT8920_REVISION_INFO* info);

// In-memory cached registers
typedef struct {
    REG_7 reg7;
    REG_FIFO_CONTROL fifo_ctrl;
    REG_STATUS status;
} LT8920_REGISTER_CACHE;
extern LT8920_REGISTER_CACHE lt8920_registers;

#ifdef __cplusplus
}
#endif

#endif	/* RS485_H */
