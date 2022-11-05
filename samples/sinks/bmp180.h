#ifndef _BPM180_H_SINK_
#define	_BPM180_H_SINK_

// BPM180 I2C module to read barometric data (air pressure)

#define SINK_BMP180_ID "BM18"

void bmp180_init();

// Reset, check device ID and download calibration data
void bmp180_resetGetCalibData();
// Read temperature and pression raw data
void bmp180_readTempPressureData();
// Returns 1 then idle
__bit bmp180_poll();

// For bus protocol interface
__bit bmp180_sinkWrite();
__bit bmp180_sinkRead();

// Access this buffer immediately after a read operation to find data
typedef union {
    uint8_t calibData[22];
    struct {
        uint8_t tempData[2];
        uint8_t pressureData[3];
        // Send buffer should not overwrite intermediate data below
        uint8_t sendBuffer[2];
    };
} BMP180_BUFFER;
extern BMP180_BUFFER bmp180_buffer;

#endif	

