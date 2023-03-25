#include <net/net.h>
#include "samples.h"

/**
 * Holding registers [0-2]
 */
typedef struct {
    /**
     * See SYS_RESET_REASON
     */
    uint8_t resetReason;
    uint8_t _filler1;
    
    /**
     * Count of CRC errors in the reading period
     */
    uint8_t crcErrors;
    uint8_t _filler2;
} SYS_REGISTERS;

#define SYS_REGS_ADDRESS (0)
#define SYS_REGS_ADDRESS_BE (LE_TO_BE_16(SYS_REGS_ADDRESS))
#define SYS_REGS_COUNT (sizeof(SYS_REGISTERS) / 2)

void samples_init() {
#ifdef HAS_LED_BLINK
    blinker_init();
#endif
#ifdef HAS_I2C
    i2c_init();
#endif
#ifdef HAS_BMP180
    bmp180_init();
#endif
}

void samples_poll() {
#ifdef HAS_LED_BLINK
    blinker_poll();
#endif
#ifdef HAS_I2C
    i2c_poll();
#endif
#ifdef HAS_BMP180
    bmp180_poll();
#endif
}

static uint16_t addressBe;

_Bool regs_validateAddr() {
    uint8_t count = bus_cl_header.address.countL;
    addressBe = bus_cl_header.address.registerAddressBe;
    
    // Exposes the system registers in the rane 0-2
    if (addressBe == SYS_REGS_ADDRESS_BE) {
        if (count != SYS_REGS_COUNT) {
            bus_cl_exceptionCode = ERR_INVALID_SIZE;
            return false;
        }
        if (bus_cl_header.header.function != READ_HOLDING_REGISTERS) {
            bus_cl_exceptionCode = ERR_INVALID_FUNCTION;
            return false;
        }
        return true;
    }
    
#ifdef HAS_LED_BLINK
    if (addressBe == LEDBLINK_REGS_ADDRESS_BE) {
        if (count != LEDBLINK_REGS_COUNT) {
            bus_cl_exceptionCode = ERR_INVALID_SIZE;
            return false;
        }
        return true;
    }
#endif
    
#ifdef HAS_BMP180
    if (addressBe == BMP180_REGS_CALIB_ADDRESS_BE) {
        if (count != BMP180_REGS_CALIB_COUNT) {
            bus_cl_exceptionCode = ERR_INVALID_SIZE;
            return false;
        }
        if (bus_cl_header.header.function != READ_HOLDING_REGISTERS) {
            bus_cl_exceptionCode = ERR_INVALID_FUNCTION;
            return false;
        }
        return true;
    }
    if (addressBe == BMP180_REGS_DATA_ADDRESS_BE) {
        if (count != BMP180_REGS_DATA_COUNT) {
            bus_cl_exceptionCode = ERR_INVALID_SIZE;
            return false;
        }
        if (bus_cl_header.header.function != READ_HOLDING_REGISTERS) {
            bus_cl_exceptionCode = ERR_INVALID_FUNCTION;
            return false;
        }
        return true;
    }
#endif
    
    bus_cl_exceptionCode = ERR_INVALID_ADDRESS;
    return false;
}

_Bool regs_onReceive() {
    if (addressBe == SYS_REGS_ADDRESS_BE) {
        // Ignore data, reset flags and counters
        sys_resetReason = RESET_NONE;
        bus_cl_crcErrors = 0;
        return true;
    }
#ifdef HAS_LED_BLINK
    if (addressBe == LEDBLINK_REGS_ADDRESS_BE) {
        memcpy(&blinker_regs, rs485_buffer, sizeof(LedBlinkRegsiters));
        return blinker_conf();
    }
#endif
    return false;
}

void regs_onSend() {
    if (addressBe == SYS_REGS_ADDRESS_BE) {
        ((SYS_REGISTERS*)rs485_buffer)->crcErrors = bus_cl_crcErrors;
        ((SYS_REGISTERS*)rs485_buffer)->resetReason = sys_resetReason;
        return;
    }
    
#ifdef HAS_LED_BLINK
    if (addressBe == LEDBLINK_REGS_ADDRESS_BE) {
        memcpy(rs485_buffer, &blinker_regs, sizeof(LedBlinkRegsiters));
        return;
    }
#endif
    
#ifdef HAS_BMP180
    if (addressBe == BMP180_REGS_CALIB_ADDRESS_BE) {
        bmp180_readCalibrationData();
        return;
    }
    if (addressBe == BMP180_REGS_DATA_ADDRESS_BE) {
        bmp180_readRawData();
        return;
    }
#endif
}

