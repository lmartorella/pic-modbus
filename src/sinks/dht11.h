#ifndef DHT11_H
#define	DHT11_H

#ifdef HAS_DHT11

void dht11_init();
#define SINK_DHT11_ID "TEMP"
bit dht11_write();

#endif

#endif	/* DHT11_H */

