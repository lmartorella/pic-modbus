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
 * Poll for bus client activities.
 * Returns true if the node is currently active and requires short polling.
 * Returns false if the node is not currently active and it can be polled with larger period,
 * depending on the medium implementation (e.g. 1ms)
 */
__bit bus_cl_poll();

/***
 ***
 * Specific state for RTU client
 ***
 **/

typedef enum {
    // The client bus is idle, waiting for a complete frame header
    BUS_CL_RTU_IDLE,
    // Read mode: skipping input data and waiting for the next idle state.
    BUS_CL_RTU_WAIT_FOR_IDLE,
    // Wait the checksum to validate the message
    BUS_CL_RTU_CHECK_REQUEST_CRC,
    // Wait the end of the packet and then start response
    BUS_CL_RTU_WAIT_FOR_RESPONSE,
    // Transmit response
    BUS_CL_RTU_RESPONSE,
    // Wait for the byte of data size
    BUS_CL_RTU_RECEIVE_DATA_SIZE,
    // Wait for the data to be received 
    BUS_CL_RTU_RECEIVE_DATA,
    // The function write function is piped to the "Read Register" function response
    BUS_CL_RTU_SEND_DATA,
    // When the response is completed and the response CRC should be written
    BUS_CL_RTU_WRITE_RESPONSE_CRC,
    // Wait for the RS485 module to end the transmission
    BUS_CL_RTU_WAIT_FOR_FLUSH
} BUS_CL_RTU_STATE;
extern BUS_CL_RTU_STATE bus_cl_rtu_state;
extern uint8_t bus_cl_crcErrors;

typedef enum {
    NO_ERROR = 0,
    ERR_INVALID_FUNCTION = 1,
    ERR_INVALID_ADDRESS = 2,
    ERR_INVALID_SIZE = 3,

    ERR_DEVICE_FAILURE = 4,
    ERR_DEVICE_ACK = 5,
            
    // Used by applicative code
    ERR_DEVICE_BUSY = 6,
    ERR_DEVICE_NACK = 7
} BUL_CL_RTU_EXCEPTION_CODE;

#define READ_HOLDING_REGISTERS (3)
#define WRITE_HOLDING_REGISTERS (16)

// If != NO_ERR, write an error
extern uint8_t bus_cl_exceptionCode;

// The very header of every Modbus message
typedef struct {
    uint8_t stationAddress;
    uint8_t function;
} ModbusRtuPacketHeader;

// Used in both reading and writing holding registers
typedef struct {
    union {
        struct {
            uint8_t registerAddressH;     // in big-endian
            uint8_t registerAddressL;     // in big-endian
        };
        uint16_t registerAddressBe;     // in big-endian
    };
    union {
        struct {
            uint8_t countH;     // in big-endian
            uint8_t countL;     // in big-endian
        };
        uint16_t countBe;               // in big-endian
    };
} ModbusRtuHoldingRegisterData;

typedef struct {
    ModbusRtuPacketHeader header;
    ModbusRtuHoldingRegisterData address;
} ModbusRtuHoldingRegisterRequest;

/**
 * Header of the last request received. It is valid for `regs_validateReg` processing and also during `regs_onReceive`
 * and `regs_onSend`
 */
extern ModbusRtuHoldingRegisterRequest bus_cl_header;

/**
 * Validate request of read/write a register range. Must validate address and size.
 * Header to check: `bus_cl_header`. Errors must be set to `bus_cl_exceptionCode`
 */
_Bool regs_validateReg();

/**
 * Called when a range of registers (sys or app) is about to be written.
 * Packet header data is in `bus_cl_header`.
 * The `rs485_buffer` contains the data to write into the holding register(s).
 * Return true for no errors. Returns false and set `bus_cl_exceptionCode` in case of errors.
 */
_Bool regs_onReceive();

/**
 * Called when the holding registers are about to be read (sent out). 
 * Packet header data is in `bus_cl_header`.
 * The `rs485_buffer` should be filled with the register content.
 */
void regs_onSend();

#ifdef __cplusplus
}
#endif

#endif	/* BUS_SEC_H */
