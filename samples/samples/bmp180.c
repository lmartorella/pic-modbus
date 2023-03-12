#include <net/net.h>
#include "./bmp180.h"
#include "../hardware/i2c.h"

// BPM180 I2C module to read barometric data (air pressure)

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

// Shared BPM180 state machine (used by app and sink)
static enum {
    STATE_IDLE,
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

// Command to perform for sink protocol READ operations
static enum {
    COMMAND_READ_DATA,
    COMMAND_RESET_READ_CALIB
} s_lastCommand;

// Access this buffer immediately after a read operation to find data
typedef struct {
    uint8_t calibData[22];
} BMP180_CALIBRATION_DATA;

typedef struct {
    uint8_t tempData[2];
    uint8_t pressureData[3];
} BMP180_READING_DATA;

static union { 
    BMP180_CALIBRATION_DATA calibration;
    BMP180_READING_DATA readings;
} bmp180_buffer;

static TICK_TYPE s_lastTime;
static uint8_t sendBuffer[2];

void bmp180_init() {
    s_state = STATE_IDLE;
    s_lastCommand = COMMAND_READ_DATA;
}

static void bmp180_resetGetCalibData() {
    if (s_state != STATE_IDLE) {
        bus_cl_exceptionCode = ERR_DEVICE_BUSY;
        return;
    }
    // TODO: reset too
    sendBuffer[0] = ADDR_DEVICE_ID;
    i2c_sendReceive7(REG_WRITE, 1, sendBuffer);
    s_state = STATE_ASK_ID;
    s_lastTime = timers_get();
}

static void bmp180_readTempPressureData() {
    if (s_state != STATE_IDLE) {
        bus_cl_exceptionCode = ERR_DEVICE_BUSY;
        return;
    }
    sendBuffer[0] = ADDR_MSG_CONTROL;
    sendBuffer[1] = MESSAGE_READ_TEMP;
    i2c_sendReceive7(REG_WRITE, 2, sendBuffer);
    s_state = STATE_ASK_TEMP;
    s_lastTime = timers_get();
}

// Returns 1 if idle
void bmp180_poll() {
    if ((timers_get() - s_lastTime) > TICKS_PER_SECOND) {
        fatal(ERR_DEVICE_DEADLINE_MISSED);
    }

    // Wait for I2C to finish previous operation
    _Bool idle = i2c_poll();
    if (!idle) {
        return;
    }
    
    switch (s_state) {
        case STATE_ASK_ID:
            // Start read buffer
            i2c_sendReceive7(REG_READ, 1, sendBuffer);
            s_state = STATE_RCV_ID;
            break;
        case STATE_RCV_ID:
            if (sendBuffer[0] != 0x55) {
                fatal(ERR_DEVICE_HW_FAIL);
            }

            // Ask table
            sendBuffer[0] = ADDR_CALIB0;
            i2c_sendReceive7(REG_WRITE, 1, sendBuffer);
            s_state = STATE_ASK_CALIB_TABLE;
            break;
            
        case STATE_ASK_CALIB_TABLE:
            // Start read table
            i2c_sendReceive7(REG_READ, 22, bmp180_buffer.calibration.calibData);
            s_state = STATE_WAIT_DATA;
            break;

        case STATE_ASK_TEMP:
            // Wait 4.5ms
            if ((timers_get() - s_lastTime) > (TICKS_PER_MILLISECOND * 5)) {
                sendBuffer[0] = ADDR_MSB;
                i2c_sendReceive7(REG_WRITE, 1, sendBuffer);
                s_state = STATE_ASK_TEMP_2;
            }
            break;
        case STATE_ASK_TEMP_2:
            // Read results
            i2c_sendReceive7(REG_READ, 2, bmp180_buffer.readings.tempData);
            s_state = STATE_WAIT_TEMP;
            break;
        case STATE_WAIT_TEMP:
            // Ask pressure (max sampling)
            sendBuffer[0] = ADDR_MSG_CONTROL;
            sendBuffer[1] = MESSAGE_READ_PRESS_OSS_3;
            i2c_sendReceive7(REG_WRITE, 2, sendBuffer);
            s_state = STATE_ASK_PRESS;
            s_lastTime = timers_get();
            break;

        case STATE_ASK_PRESS:   
            // Wait 25ms (OSS = 3)
            if ((timers_get() - s_lastTime) > (TICKS_PER_MILLISECOND * 30)) {
                sendBuffer[0] = ADDR_MSB;
                i2c_sendReceive7(REG_WRITE, 1, sendBuffer);
                s_state = STATE_ASK_PRESS_2;
            }
            break;
        case STATE_ASK_PRESS_2:
            // Read results
            i2c_sendReceive7(REG_READ, 3, bmp180_buffer.readings.pressureData);
            s_state = STATE_WAIT_DATA;
            break;

        case STATE_WAIT_DATA:   
            s_state = STATE_IDLE;
            // Buffer ready!
            break;
    }
}

void bmp180_read_reg1() {    
    if (s_state != STATE_IDLE) {
        bus_cl_exceptionCode = ERR_DEVICE_BUSY;
        return;
    }
    memcpy(bus_cl_buffer_le16, (uint8_t*)&bmp180_buffer.readings, sizeof(bmp180_buffer.readings));
}

void bmp180_read_reg2() {    
    if (s_state != STATE_IDLE) {
        bus_cl_exceptionCode = ERR_DEVICE_BUSY;
        return;
    }
    memcpy(bus_cl_buffer_le16, (uint8_t*)&bmp180_buffer.calibration, 16);
}

void bmp180_read_reg3() {    
    if (s_state != STATE_IDLE) {
        bus_cl_exceptionCode = ERR_DEVICE_BUSY;
        return;
    }
    memcpy(bus_cl_buffer_le16, ((uint8_t*)&bmp180_buffer.calibration) + 16, sizeof(bmp180_buffer.calibration) - 16);
}

void bmp180_write_reg1() {
    s_lastCommand = (uint8_t)bus_cl_buffer_le16[0];
    // Ensure valid enum
    if (s_lastCommand != COMMAND_RESET_READ_CALIB) {
        s_lastCommand = COMMAND_READ_DATA;
        bmp180_readTempPressureData();
    } else {
        bmp180_resetGetCalibData();
    }
}
