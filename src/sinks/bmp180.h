#ifndef _BPM180_H_SINK_
#define	_BPM180_H_SINK_

#define SINK_BMP180_ID "BM18"

void bmp180_init();

// Reset, check device ID and download calibration data
void bmp180_resetGetCalibData();
// Read temperature and pression raw data
void bmp180_readTempPressureData();
// Returns 1 then idle
bit bmp180_poll();

// For bus protocol interface
bit bmp180_sinkWrite();
bit bmp180_sinkRead();

// Access this buffer immediately after a read operation to find data
typedef union {
    BYTE calibData[22];
    struct {
        BYTE tempData[2];
        BYTE pressureData[3];
        // Send buffer should not overwrite intermediate data below
        BYTE sendBuffer[2];
    };
} BMP180_BUFFER;
extern BMP180_BUFFER bmp180_buffer;

#endif	

