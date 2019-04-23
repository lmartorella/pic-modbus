#ifndef _BPM180_NET_H
#define _BPM180_NET_H

// BPM180 I2C module to read barometric data (air pressure)

#ifdef HAS_BMP180_APP

void bmp180_app_init();
void bmp180_app_poll();

#endif

#endif