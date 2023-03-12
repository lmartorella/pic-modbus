#ifndef _BPM180_H_SINK_
#define	_BPM180_H_SINK_

// BPM180 I2C module to read barometric data (air pressure)

#define SINK_BMP180_ID "BM18"

void bmp180_init();

// Returns 1 then idle
void bmp180_poll();

// For bus protocol interface. Uses 3 function address space (8*3 = 24 registers)
void bmp180_read_reg1();
void bmp180_read_reg2();
void bmp180_read_reg3();
void bmp180_write_reg1();

#endif	

