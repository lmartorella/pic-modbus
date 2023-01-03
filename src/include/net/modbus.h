#ifndef _MODBUS_H
#define	_MODBUS_H

/**
 * Common interface for Modbus protocol.
 */

/**
 * The station address range is 1-247. 
 * It is 0 if the station still doesn't have an address (auto-configuration).
 */
#define UNASSIGNED_STATION_ADDRESS (0)

//void prot_init();

// Has the slow timer ticked?
// extern __bit prot_slowTimer;
// extern __bit prot_registered;

// /**
//  * Implementation dependent calls
//  */
// // Directly with define in order to minimize stack usage
// #define prot_sec_control_readW(w) rs485_read((uint8_t*)w, 2) 
// #define prot_sec_control_read(data, size) rs485_read((uint8_t*)data, (uint8_t)size)
// #define prot_sec_control_writeW(w) rs485_write(false, (uint8_t*)&w, 2)
// #define prot_sec_control_write(data, size) rs485_write(false, (uint8_t*)data, (uint8_t)size)
// #define prot_sec_control_over() set_rs485_over()
// #define prot_sec_control_idle(buf) rs485_write(true, buf, 1)
// #define prot_sec_control_readAvail() rs485_readAvail()
// #define prot_sec_control_writeAvail() rs485_writeAvail()
// #define prot_sec_control_isConnected() bus_sec_isConnected()
// #define prot_sec_control_close() bus_sec_abort()

// __bit prot_prim_control_readW(uint16_t* w);
// __bit prot_prim_control_read(void* data, uint16_t size);
// void prot_prim_control_writeW(uint16_t w);
// void prot_prim_control_write(const void* data, uint16_t size);
// void prot_prim_control_over(void);
// #define prot_prim_control_idle()
// uint16_t prot_prim_control_readAvail(void);
// uint16_t prot_prim_control_writeAvail(void);
// #define prot_prim_control_isConnected() ip_isConnected()
// #define prot_prim_control_close() ip_flush()
// void prot_prim_control_abort(void);


// #if defined(HAS_RS485_BUS_PRIMARY)
// #define prot_control_writeAvail prot_prim_control_writeAvail
// #define prot_control_write prot_prim_control_write
// #define prot_control_writeW prot_prim_control_writeW
// #define prot_control_readAvail prot_prim_control_readAvail
// #define prot_control_read prot_prim_control_read
// #define prot_control_readW prot_prim_control_readW
// #elif defined(HAS_RS485_BUS_SECONDARY)
// #define prot_control_writeAvail prot_sec_control_writeAvail
// #define prot_control_write prot_sec_control_write
// #define prot_control_writeW prot_sec_control_writeW
// #define prot_control_readAvail prot_sec_control_readAvail
// #define prot_control_read prot_sec_control_read
// #define prot_control_readW prot_sec_control_readW
// #endif

#endif	/* PROTOCOL_H */
