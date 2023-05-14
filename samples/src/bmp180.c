#include <pic-modbus/modbus.h>
#include "./bmp180.h"
#include "./hardware/i2c.h"

#ifdef HAS_BMP180

// BMP180 I2C module to read barometric data (air pressure)

#define REG_READ 0xef
#define REG_WRITE 0xee

#define MESSAGE_READ_TEMP 0x2e
#define MESSAGE_READ_PRESS_OSS_0 0x34
#define MESSAGE_READ_PRESS_OSS_1 0x74
#define MESSAGE_READ_PRESS_OSS_2 0xb4
#define MESSAGE_READ_PRESS_OSS_3 0xf4

#define ADDR_CALIB0 0xaa
#define ADDR_CALIB21 0xbf
#define ADDR_DEVICE_ID 0xd0
#define ADDR_MSG_CONTROL 0xf4
#define ADDR_MSB 0xf6
#define ADDR_LSB 0xf7
#define ADDR_XLSB 0xf8

// Shared BMP180 state machine (used by app and sink)
static enum {
    STATE_IDLE,
    STATE_RESET,
    STATE_ASK_ID,
    STATE_RCV_ID,
    STATE_ASK_CALIB_TABLE,
    STATE_ASK_TEMP,
    STATE_ASK_TEMP_2,
    STATE_WAIT_TEMP,
    STATE_ASK_PRESS,
    STATE_ASK_PRESS_2,

    STATE_WAIT_DATA,
} s_state;

// Access this buffer immediately after a read operation to find data
static uint8_t calibrationData[22];

static struct {
    uint8_t temperature[2];
    uint8_t pressure[3];
} rawData;

static TICK_TYPE s_lastTime;
static uint8_t i2cBuffer[2];

void bmp180_init() {
    s_state = STATE_RESET;
}

static void resetGetCalibData() {
    // TODO: reset too
    i2cBuffer[0] = ADDR_DEVICE_ID;
    i2c_sendReceive7(REG_WRITE, 1, i2cBuffer);
    s_state = STATE_ASK_ID;
    s_lastTime = timers_get();
}

static void readRawData() {
    i2cBuffer[0] = ADDR_MSG_CONTROL;
    i2cBuffer[1] = MESSAGE_READ_TEMP;
    i2c_sendReceive7(REG_WRITE, 2, i2cBuffer);
    s_state = STATE_ASK_TEMP;
    s_lastTime = timers_get();
}

// Returns 1 if idle
void bmp180_poll() {
    if ((timers_get() - s_lastTime) > TICKS_PER_SECOND) {
        sys_fatal(ERR_DEVICE_DEADLINE_MISSED);
    }

    // Wait for I2C to finish previous operation
    _Bool idle = i2c_poll();
    if (!idle) {
        return;
    }
    
    switch (s_state) {
        case STATE_RESET:
            resetGetCalibData();
            break;
        case STATE_ASK_ID:
            // Start read buffer
            i2c_sendReceive7(REG_READ, 1, i2cBuffer);
            s_state = STATE_RCV_ID;
            break;
        case STATE_RCV_ID:
            if (i2cBuffer[0] != 0x55) {
                sys_fatal(ERR_DEVICE_HW_FAIL);
            }

            // Ask table
            i2cBuffer[0] = ADDR_CALIB0;
            i2c_sendReceive7(REG_WRITE, 1, i2cBuffer);
            s_state = STATE_ASK_CALIB_TABLE;
            break;
            
        case STATE_ASK_CALIB_TABLE:
            // Start read table
            i2c_sendReceive7(REG_READ, 22, &calibrationData);
            s_state = STATE_WAIT_DATA;
            break;

        case STATE_ASK_TEMP:
            // Wait 4.5ms
            if ((timers_get() - s_lastTime) > (TICKS_PER_MILLISECOND * 5)) {
                i2cBuffer[0] = ADDR_MSB;
                i2c_sendReceive7(REG_WRITE, 1, i2cBuffer);
                s_state = STATE_ASK_TEMP_2;
            }
            break;
        case STATE_ASK_TEMP_2:
            // Read results
            i2c_sendReceive7(REG_READ, 2, &rawData.temperature);
            s_state = STATE_WAIT_TEMP;
            break;
        case STATE_WAIT_TEMP:
            // Ask pressure (max sampling)
            i2cBuffer[0] = ADDR_MSG_CONTROL;
            i2cBuffer[1] = MESSAGE_READ_PRESS_OSS_3;
            i2c_sendReceive7(REG_WRITE, 2, i2cBuffer);
            s_state = STATE_ASK_PRESS;
            s_lastTime = timers_get();
            break;

        case STATE_ASK_PRESS:   
            // Wait 25ms (OSS = 3)
            if ((timers_get() - s_lastTime) > (TICKS_PER_MILLISECOND * 30)) {
                i2cBuffer[0] = ADDR_MSB;
                i2c_sendReceive7(REG_WRITE, 1, i2cBuffer);
                s_state = STATE_ASK_PRESS_2;
            }
            break;
        case STATE_ASK_PRESS_2:
            // Read results
            i2c_sendReceive7(REG_READ, 3, &rawData.pressure);
            s_state = STATE_WAIT_DATA;
            break;

        case STATE_WAIT_DATA:   
            s_state = STATE_IDLE;
            s_lastTime = timers_get();
            // Buffer ready!
            break;
            
        case STATE_IDLE:
            // Read data twice per second
            if ((timers_get() - s_lastTime) > (TICKS_PER_SECOND / 2)) {
                readRawData();
            }
            break;
    }
}

void bmp180_readRawData() {    
    if (s_state != STATE_IDLE) {
        bus_cl_exceptionCode = ERR_DEVICE_BUSY;
        return;
    }
    memcpy(rs485_buffer, (uint8_t*)&rawData, sizeof(rawData));
}

void bmp180_readCalibrationData() {    
    if (s_state != STATE_IDLE) {
        bus_cl_exceptionCode = ERR_DEVICE_BUSY;
        return;
    }
    memcpy(rs485_buffer, (uint8_t*)&calibrationData, sizeof(calibrationData));
}

#endif
