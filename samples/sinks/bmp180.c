#include <net/net.h>
#include "./bmp180.h"
#include "../hardware/i2c.h"

// BPM180 I2C module to read barometric data (air pressure)
#ifdef HAS_BMP180

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
    COMMAND_RESET_READ_CALIB = 22, // data size
    COMMAND_READ_DATA = 5 // data size
} s_lastCommand;

// Sink state
#define SINK_STATE_IDLE (0)
static signed char s_sinkStateOrSize;
static uint8_t s_ptr;

BMP180_BUFFER bmp180_buffer;
static TICK_TYPE s_lastTime;

void bmp180_init() {
    s_state = STATE_IDLE;
    s_lastCommand = COMMAND_READ_DATA;
    s_sinkStateOrSize = SINK_STATE_IDLE;
}

void bmp180_resetGetCalibData() {
    if (s_state != STATE_IDLE) {
        fatal("B.BSY");
    }
    // TODO: reset too
    bmp180_buffer.sendBuffer[0] = ADDR_DEVICE_ID;
    i2c_sendReceive7(REG_WRITE, 1, bmp180_buffer.sendBuffer);
    s_state = STATE_ASK_ID;
    s_lastTime = timers_get();
}

void bmp180_readTempPressureData() {
    if (s_state != STATE_IDLE) {
        fatal("B.BSY");
    }
    bmp180_buffer.sendBuffer[0] = ADDR_MSG_CONTROL;
    bmp180_buffer.sendBuffer[1] = MESSAGE_READ_TEMP;
    i2c_sendReceive7(REG_WRITE, 2, bmp180_buffer.sendBuffer);
    s_state = STATE_ASK_TEMP;
    s_lastTime = timers_get();
}

// Returns 1 if idle
__bit bmp180_poll() {
    if (s_state == STATE_IDLE) {
        return 1;
    }
    if ((timers_get() - s_lastTime) > TICK_SECOND) {
        fatal("B.LOCK");
    }

    // Wait for I2C to finish previous operation
    //int oss = 3;
    _Bool idle = i2c_poll();
    if (!idle) {
        return 0;
    }
    
    switch (s_state) {
        case STATE_ASK_ID:
            // Start read buffer
            i2c_sendReceive7(REG_READ, 1, bmp180_buffer.sendBuffer);
            s_state = STATE_RCV_ID;
            break;
        case STATE_RCV_ID:
            if (bmp180_buffer.sendBuffer[0] != 0x55) {
                fatal("B.ID");
            }

            // Ask table
            bmp180_buffer.sendBuffer[0] = ADDR_CALIB0;
            i2c_sendReceive7(REG_WRITE, 1, bmp180_buffer.sendBuffer);
            s_state = STATE_ASK_CALIB_TABLE;
            break;
            
        case STATE_ASK_CALIB_TABLE:
            // Start read table
            i2c_sendReceive7(REG_READ, 22, bmp180_buffer.calibData);
            s_state = STATE_WAIT_DATA;
            break;

        case STATE_ASK_TEMP:
            // Wait 4.5ms
            if ((timers_get() - s_lastTime) > (TICKS_PER_MILLISECOND * 5)) {
                bmp180_buffer.sendBuffer[0] = ADDR_MSB;
                i2c_sendReceive7(REG_WRITE, 1, bmp180_buffer.sendBuffer);
                s_state = STATE_ASK_TEMP_2;
            }
            break;
        case STATE_ASK_TEMP_2:
            // Read results
            i2c_sendReceive7(REG_READ, 2, bmp180_buffer.tempData);
            s_state = STATE_WAIT_TEMP;
            break;
        case STATE_WAIT_TEMP:
            // Ask pressure (max sampling)
            bmp180_buffer.sendBuffer[0] = ADDR_MSG_CONTROL;
            bmp180_buffer.sendBuffer[1] = MESSAGE_READ_PRESS_OSS_3;
            i2c_sendReceive7(REG_WRITE, 2, bmp180_buffer.sendBuffer);
            s_state = STATE_ASK_PRESS;
            s_lastTime = timers_get();
            break;

        case STATE_ASK_PRESS:   
            // Wait 25ms (OSS = 3)
            if ((timers_get() - s_lastTime) > (TICKS_PER_MILLISECOND * 30)) {
                bmp180_buffer.sendBuffer[0] = ADDR_MSB;
                i2c_sendReceive7(REG_WRITE, 1, bmp180_buffer.sendBuffer);
                s_state = STATE_ASK_PRESS_2;
            }
            break;
        case STATE_ASK_PRESS_2:
            // Read results
            i2c_sendReceive7(REG_READ, 3, bmp180_buffer.pressureData);
            s_state = STATE_WAIT_DATA;
            break;

        case STATE_WAIT_DATA:   
            s_state = STATE_IDLE;
            // Buffer ready!
            return true;
    }
    return false;
}

__bit bmp180_sinkWrite() {
    // Wait for BMP180 idle (shared with app)
    if (!bmp180_poll()) {
        // Wait for data
        return 1;
    }
    
    // Do readings
    if (s_sinkStateOrSize == SINK_STATE_IDLE) {
        // Start acquisition
        switch (s_lastCommand) {
            case COMMAND_RESET_READ_CALIB:
                bmp180_resetGetCalibData();
                s_sinkStateOrSize = sizeof(bmp180_buffer.calibData);
                break;
            case COMMAND_READ_DATA:
            default:
                bmp180_readTempPressureData();
                s_sinkStateOrSize = sizeof(bmp180_buffer.pressureData) + sizeof(bmp180_buffer.tempData);
                break;
        }
        s_ptr = 0;
        // Data will follow
        return 1;
    } else if (s_sinkStateOrSize > 0) {
        uint8_t l = prot_control_writeAvail();
        if ((uint8_t)s_sinkStateOrSize < l) {
            l = (uint8_t)s_sinkStateOrSize;
        }
        prot_control_write(((uint8_t*)&bmp180_buffer) + s_ptr, l);
        s_ptr += l;
        s_sinkStateOrSize -= l;
        return s_sinkStateOrSize > 0;
    } else {
        // Done
        return 0;
    }
}

__bit bmp180_sinkRead() {
    if (prot_control_readAvail() < 1) {
        return 1;        
    }
    prot_control_read(&s_lastCommand, 1);
    // Ensure valid enum
    if (s_lastCommand != COMMAND_RESET_READ_CALIB) {
        s_lastCommand = COMMAND_READ_DATA;
    }
    s_sinkStateOrSize = SINK_STATE_IDLE;
    return 0;
}

#endif
