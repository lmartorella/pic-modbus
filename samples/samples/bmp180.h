#ifndef _BPM180_H_SINK_
#define	_BPM180_H_SINK_

#include "./sample_config.h"

#ifdef	__cplusplus
extern "C" {
#endif

// BPM180 I2C module to read barometric data (air pressure)

#define BMP180_REGS_CALIB_ADDRESS (1024)
#define BMP180_REGS_CALIB_COUNT (11)
#define BMP180_REGS_DATA_ADDRESS (1024 + 11)
#define BMP180_REGS_DATA_COUNT (3)

#define BMP180_REGS_CALIB_ADDRESS_BE (LE_TO_BE_16(BMP180_REGS_CALIB_ADDRESS))
#define BMP180_REGS_DATA_ADDRESS_BE (LE_TO_BE_16(BMP180_REGS_DATA_ADDRESS))

void bmp180_init();

// Returns 1 then idle
void bmp180_poll();

// Load registers with the right data
void bmp180_readRawData();
void bmp180_readCalibrationData();

#ifdef	__cplusplus
}
#endif

#endif	

