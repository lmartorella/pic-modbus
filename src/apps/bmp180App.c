#include "../pch.h"
#include "../sinks/bmp180.h"
#include "../appio.h"

// BPM180 I2C module to read barometric data (air pressure)
#ifdef HAS_BMP180_APP

static struct {
    short ac1;
    short ac2;
    short ac3;
    unsigned short ac4;
    unsigned short ac5;
    unsigned short ac6;
    short b1;
    short b2;
    short mb;
    short mc;
    short md;
} s_calibTable;

// app state machine
static enum {
    STATE_IDLE,
    STATE_CALIB_TABLE,
    STATE_TEMPPRESS
} s_state;

static TICK_TYPE s_lastTime;
static BYTE s_count;

void bmp180_app_init() {
    bmp180_resetGetCalibData();
    s_state = STATE_CALIB_TABLE;
}

void bmp180_app_poll() {
    int oss = 3;
    BOOL idle = bmp180_poll();
    if (!idle) {
        return; 
    }
    
    switch (s_state) {
        case STATE_CALIB_TABLE:   
            {
                memcpy(&s_calibTable, &bmp180_buffer.calibData, sizeof(bmp180_buffer.calibData));
                // Change endianness
                for (int i = 0; i < 11; i++) {
                    BYTE b1 = ((BYTE*)&s_calibTable)[i * 2];
                    ((BYTE*)&s_calibTable)[i * 2] = ((BYTE*)&s_calibTable)[i * 2 + 1];
                    ((BYTE*)&s_calibTable)[i * 2 + 1] = b1;
                }

                s_state = STATE_IDLE;
                s_lastTime = TickGet();
            }
            break;
        case STATE_IDLE:
            // Start reading?
            if ((TickGet() - s_lastTime) > (TICK_SECOND * 2)) {
                bmp180_readTempPressureData();
                s_state = STATE_TEMPPRESS;
            }
            break;
        case STATE_TEMPPRESS:
            {
                // Uncompensate temp
                unsigned long s_ut = ((unsigned long)(bmp180_buffer.tempData[0]) << 8) | (unsigned long)bmp180_buffer.tempData[1];
                // Uncompensate pressure
                unsigned long s_up = (((unsigned long)bmp180_buffer.pressureData[0] << 16) |
                       ((unsigned long)bmp180_buffer.pressureData[1] << 8) | 
                        (unsigned long)bmp180_buffer.pressureData[2]) >> (8 - oss);

                float temp, press;
                long b5;
                {
                    // Calc true temp
                    long x1 = (((long)s_ut - (long)s_calibTable.ac6) * (long)s_calibTable.ac5) >> 15;
                    if (x1 == 0 && s_calibTable.md == 0) {
                        fatal("B.DIV");
                    }
                    long x2 = ((long)s_calibTable.mc << 11) / (x1 + s_calibTable.md);
                    b5 = x1 + x2;
                    short t = (b5 + 8) >> 4;  // Temp in 0.1C
                    temp = t / 10.0;
                }
                {
                    long b6 = b5 - 4000;
                    long x1 = ((long)s_calibTable.b2 * ((b6 * b6) >> 12)) >> 11;
                    long x2 = ((long)s_calibTable.ac2 * b6) >> 11;
                    long x3 = x1 + x2;
                    long b3 = ((((long)s_calibTable.ac1 * 4 + x3) << oss) + 2) >> 2;
                    x1 = ((long)s_calibTable.ac3 * b6) >> 13;
                    x2 = ((long)s_calibTable.b1 * ((b6 * b6) >> 12)) >> 16;
                    x3 = ((x1 + x2) + 2) >> 2;
                    unsigned long b4 = ((long)s_calibTable.ac4 * (unsigned long)(x3 + 32768)) >> 15;
                    unsigned long b7 = ((unsigned long)(s_up - b3) * (50000ul >> oss));
                    if (b4 == 0) {
                        fatal("B.DIV2");
                    }
                    long p;
                    if (b7 < 0x80000000) {
                        p = (b7 << 1) / b4; 
                    } else {
                        p = (b7 / b4) << 1;
                    }
                    x1 = (((p >> 8) * (p >> 8)) * 3038) >> 16;
                    x2 = (p * -7357) >> 16;                
                    p += (x1 + x2 + 3791) >> 4; // in Pascal
                    press = p / 100.0; // in hPa
                }
            
                BYTE buffer[16];
                sprintf(buffer, "%4.1f'C%c%6.1fhPa", temp, (s_count % 2) ? '.' : ' ', press);
                println(buffer);
            }
                        
            // DONE!
            s_state = STATE_IDLE;
            s_lastTime = TickGet();
            s_count++;
            break;
    }
}

#endif
