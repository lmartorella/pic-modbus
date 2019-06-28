#include "../../../src/nodes/pch.h"
#include "../../../src/nodes/protocol.h"
#include "dht11.h"

#ifdef HAS_DHT11

void dht11_init()
{
    // Port high, pullups, output
    DHT11_PORT_PULLUPS_INIT();
    DHT11_PORT = 1;
    DHT11_PORT_TRIS = 0;
    
    US_TIMER_INIT();
    
    // wait for sensor to stabilize
    //wait1s();
}

typedef enum {
    ERR_OK = 1,
    ERR_MISSING_LEADING_HIGH = 2,  // Infinite level 0 after engage
    ERR_MISSING_LEADING_LOW = 3,  // Infinite level 1 after engage
    ERR_AT_BYTE = 0x10,         // Err at byte 0x10 + N
} ERR_BYTE;

ERR_BYTE dht11_read(BYTE* buffer)
{   
    DHT11_PORT = 0;     // low
    
    // Low for at least 25ms to wake up DHT11
    for (int i = 0; i < 25; i++) {
        __delay_ms(1); 
        CLRWDT();
    }
    DHT11_PORT = 1;

    // Now delay 30us waiting for the DHT11 to take the control of the bus, it will force 0
    US_TIMER = 0;
    while (US_TIMER < 30);
    
    DHT11_PORT_TRIS = 1; // Data port is input, now 0
    
    // Check response
    US_TIMER = 0;
    while (!DHT11_PORT) {
        if (US_TIMER > 200) {
            return ERR_MISSING_LEADING_HIGH;
        }
    }
    US_TIMER = 0;
    while (DHT11_PORT) {
        if (US_TIMER > 200) {
            return ERR_MISSING_LEADING_LOW;
        }
    }

    for (char i = 0; i < 5; i++, buffer++) {
        CLRWDT();
        BYTE res = 0;
        for (char j = 0; j < 8; j++)
        {
            res <<= 1;
            US_TIMER = 0;
            while (!DHT11_PORT)
                if (US_TIMER > 200) {
                    return ERR_AT_BYTE + i;
                }

            US_TIMER = 0;
            while (DHT11_PORT)
                if (US_TIMER > 200) {
                    return ERR_AT_BYTE + i;
                }

            if (US_TIMER > 40)
                res |= 1;
        }   
        *buffer = res;
    }
    
    return ERR_OK;
}

bit dht11_write() {
    BYTE data[6];
    di();    
    data[0] = dht11_read(data + 1);
    ei();
    DHT11_PORT = 1;
    DHT11_PORT_TRIS = 0;

    prot_control_write(data, 6);

    // Finish data
    return FALSE;
}

#endif