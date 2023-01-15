#ifndef _MODBUS_CLIENT_H
#define	_MODBUS_CLIENT_H

#include "configuration.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * Common interface for Modbus server (secondary) nodes.
 * It can be implemented, for instance, by RTU nodes (RS485 client), TCP clients or 
 * wireless radio client stations.
 *
 * Routers (e.g. from TCP to RTU) will run both client and server instances.
 */

/**
 * Initialize the client node
 */
void bus_cl_init();

/**
 * The current station address.
 */
extern uint8_t bus_cl_stationAddress;

/**
 * Poll for bus client activities.
 * Returns true if the node is currently active and requires short polling.
 * Returns false if the node is not currently active and it can be polled with larger period,
 * depending on the medium implementation (e.g. 1ms)
 */
__bit bus_cl_poll();

// Specific state for RTU client
typedef enum {
    // The client bus is idle, waiting for a complete frame header
    BUS_CL_RTU_IDLE,
    // Read mode: skipping input data and waiting for the next idle state.
    BUS_CL_RTU_WAIT_FOR_IDLE,
    // Read the address and size of the data to read/write
    BUS_CL_RTU_WAIT_REGISTER_DATA,
    // Wait the checksum to validate the message
    BUS_CL_RTU_CHECK_REQUEST_CRC,
    // Wait the end of the packet and then start response
    BUS_CL_RTU_WAIT_FOR_RESPONSE,
    // Transmit response
    BUS_CL_RTU_RESPONSE,
    // The sink read function is piped to the "Write Register" function request
    BUS_CL_RTU_READ_STREAM,
    // The sink write function is piped to the "Read Register" function response
    BUS_CL_RTU_WRITE_STREAM,
    // When the response is completed and the response CRC should be written
    BUS_CL_RTU_WRITE_RESPONSE_CRC,
    // Wait for the RS485 module to end the transmission
    BUS_CL_RTU_WAIT_FOR_FLUSH
} BUS_CL_RTU_STATE;
extern BUS_CL_RTU_STATE bus_cl_rtu_state;

#ifdef __cplusplus
}
#endif

#endif	/* BUS_SEC_H */
