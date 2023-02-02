#include "assert.h"
#include "endian.h"
#include "net/bus_client.h"
#include "net/crc.h"
#include "net/rs485.h"

/**
 * Specific module for client Modbus RTU nodes (RS485), optimized
 * for 8-bit CPUs with low memory.
 */

typedef struct {
    uint8_t address;
    uint8_t function;
} ModbusRtuPacketHeader;

#define READ_HOLDING_REGISTERS (3)
#define WRITE_HOLDING_REGISTERS (16)

typedef struct {
    uint8_t registerAddressH; // big endian
    uint8_t registerAddressL; // big endian
    uint8_t countH;           // big endian
    uint8_t countL;           // big endian
} ModbusRtuHoldingRegisterRequest;

// Optimize number of read operations, for Write Register read also count;
typedef struct {
    ModbusRtuHoldingRegisterRequest req;
    uint8_t countBytes;
} ModbusRtuHoldingRegisterWriteRequest;

typedef struct {
    ModbusRtuPacketHeader header;
    ModbusRtuHoldingRegisterRequest address;
} ModbusRtuPacketResponse;

typedef struct {
    ModbusRtuPacketHeader header;
    uint8_t error;
} ModbusRtuPacketErrorResponse;

typedef struct {
    ModbusRtuPacketResponse resp;
    uint8_t countBytes;
} ModbusRtuPacketReadResponse;

/**
 * The current station address. It is UNASSIGNED_STATION_ADDRESS (255) if the station still doesn't have an address (auto-configuration).
 */
uint8_t bus_cl_stationAddress;

static ModbusRtuHoldingRegisterRequest s_curRequest;

// If != NO_ERR, write an error
static uint8_t s_exceptionCode;
// Store the bytes remaining for function data streaming
static uint8_t s_sizeRemaining;
// The current function in use
static uint8_t s_function;

BUS_CL_RTU_STATE bus_cl_rtu_state;

void bus_cl_init() {
    // RS485 already in receive mode
    bus_cl_rtu_state = BUS_CL_RTU_IDLE;
}

// Called often
__bit bus_cl_poll() {
    if (rs485_isMarkCondition && bus_cl_rtu_state != BUS_CL_RTU_IDLE) {
        if (bus_cl_rtu_state != BUS_CL_RTU_WAIT_FOR_RESPONSE) {
            // Abort reading, go idle
            bus_cl_rtu_state = BUS_CL_RTU_IDLE;
            return false;
        } else {
            bus_cl_rtu_state = BUS_CL_RTU_RESPONSE;
        }
    }

    if (bus_cl_rtu_state == BUS_CL_RTU_IDLE) {
        // Else decode the packet header
        if (rs485_readAvail() < sizeof(ModbusRtuPacketHeader)) {
            // Nothing to do, wait for more data
            return false;
        }
        // Free the buffer
        rs485_discard(sizeof(ModbusRtuPacketHeader));

#define packet_2 ((const ModbusRtuPacketHeader*)rs485_buffer)
        if (packet_2->address == bus_cl_stationAddress) {
            s_function = packet_2->function;
            if (s_function == READ_HOLDING_REGISTERS || s_function == WRITE_HOLDING_REGISTERS) {
                // The message is for reading registers. Address data will follow
                bus_cl_rtu_state = BUS_CL_RTU_WAIT_REGISTER_DATA;
            } else {
                // Invalid function, return error
                s_exceptionCode = ERR_INVALID_FUNCTION;
                bus_cl_rtu_state = BUS_CL_RTU_WAIT_FOR_RESPONSE;
                return false;
            }
        } else {
            // No this station, wait for idle
            bus_cl_rtu_state = BUS_CL_RTU_WAIT_FOR_IDLE;
        }
    }

    if (bus_cl_rtu_state == BUS_CL_RTU_WAIT_REGISTER_DATA) {
        // In case of write, read the size too
        uint8_t messageSize = (s_function == READ_HOLDING_REGISTERS ? sizeof(ModbusRtuHoldingRegisterRequest) : sizeof(ModbusRtuHoldingRegisterRequest) + 1);
        if (rs485_readAvail() < messageSize) {
            // Nothing to do, wait for more data
            return false;
        }
        // Free the buffer
        rs485_discard(messageSize);

#define packet_1 ((const ModbusRtuHoldingRegisterWriteRequest*)rs485_buffer)
        
        s_curRequest = packet_1->req;
        // register address if the function id * 256
        if (s_curRequest.registerAddressH == 0) {
            if ((s_curRequest.registerAddressL & 0xf) != 0 || (s_curRequest.registerAddressL >> 4) >= bus_cl_sysFunctionCount) {
                // Invalid address, return error
                s_exceptionCode = ERR_INVALID_ADDRESS;
                bus_cl_rtu_state = BUS_CL_RTU_WAIT_FOR_RESPONSE;
                return false;
            }
            // Sys functions has fixed size to save program space
            s_sizeRemaining = 16;
        } else {
            if (s_curRequest.registerAddressL != 0 || s_curRequest.registerAddressH > bus_cl_appFunctionCount) {
                // Invalid address, return error
                s_exceptionCode = ERR_INVALID_ADDRESS;
                bus_cl_rtu_state = BUS_CL_RTU_WAIT_FOR_RESPONSE;
                return false;
            }
            s_sizeRemaining = (s_function == READ_HOLDING_REGISTERS) ? 
                bus_cl_appFunctionReadHandlerSizes[s_curRequest.registerAddressH - 1] : 
                bus_cl_appFunctionWriteHandlerSizes[s_curRequest.registerAddressH - 1];
        }
        if (packet_1->req.countH != 0 || packet_1->req.countL != (s_sizeRemaining >> 1) || s_sizeRemaining == 0) {
            // Invalid size, return error
            s_exceptionCode = ERR_INVALID_SIZE;
            bus_cl_rtu_state = BUS_CL_RTU_WAIT_FOR_RESPONSE;
            return false;
        }

        if (s_function == READ_HOLDING_REGISTERS) {
            // Ok, function data must be read. Wait for packet to end
            bus_cl_rtu_state = BUS_CL_RTU_CHECK_REQUEST_CRC;
        } else {
            if (packet_1->countBytes != s_sizeRemaining) {
                // Invalid size, return error
                s_exceptionCode = ERR_INVALID_SIZE;
                bus_cl_rtu_state = BUS_CL_RTU_WAIT_FOR_RESPONSE;
                return false;
            } else {
                bus_cl_rtu_state = BUS_CL_RTU_READ_STREAM;
            }
        }
    }

    if (bus_cl_rtu_state == BUS_CL_RTU_READ_STREAM) {
        uint8_t avail = rs485_readAvail();
        uint8_t remaining = s_sizeRemaining;
        // A single call can be done to read the rest of the stream?
        if (s_sizeRemaining > RS485_BUF_SIZE) {
            remaining = RS485_BUF_SIZE;
        }

        if (avail < remaining) {
            return false;
        }
        // Free the buffer
        rs485_discard(remaining);
        (*((s_curRequest.registerAddressH == 0) ? 
            &bus_cl_sysFunctionWriteHandlers[s_curRequest.registerAddressL >> 4] : 
            &bus_cl_appFunctionWriteHandlers[s_curRequest.registerAddressH - 1]))();
        s_sizeRemaining -= remaining;

        if (s_sizeRemaining > 0) {
            return false;
        } else {
            // Next state
            bus_cl_rtu_state = BUS_CL_RTU_CHECK_REQUEST_CRC;
        }
    }

    if (bus_cl_rtu_state == BUS_CL_RTU_CHECK_REQUEST_CRC) {
        // CRC is LSB first
        uint16_t expectedCrc = le16toh(crc16);

        if (rs485_readAvail() < sizeof(uint16_t)) {
            // Nothing to do, wait for more data
            return false;
        }
        // Free the buffer
        rs485_discard(sizeof(uint16_t));

        bus_cl_rtu_state = BUS_CL_RTU_WAIT_FOR_RESPONSE;
        if (expectedCrc != *((const uint16_t*)rs485_buffer)) {
            // Invalid CRC, skip data.
            // TODO: However the function data was already written if piped!
            bus_cl_rtu_state = BUS_CL_RTU_WAIT_FOR_IDLE;
        } else {
            // Ok, go on with the response
            s_exceptionCode = NO_ERROR;
        }
    }

    if (bus_cl_rtu_state == BUS_CL_RTU_WAIT_FOR_IDLE) {
        rs485_discard(rs485_readAvail());
        return false;
    }

    if (bus_cl_rtu_state == BUS_CL_RTU_RESPONSE) {
        if (s_exceptionCode == NO_ERROR) {
            // Transmit packet data in one go
#define resp_1 ((ModbusRtuPacketReadResponse*)rs485_buffer)
            resp_1->resp.header.address = bus_cl_stationAddress;
            resp_1->resp.header.function = s_function;
            // Response of read/write registers always contains the address and register count
            resp_1->resp.address = s_curRequest;
            // Now, if write, open stream
            if (s_function == READ_HOLDING_REGISTERS) {
                resp_1->countBytes = s_sizeRemaining;
                rs485_write(sizeof(ModbusRtuPacketReadResponse));
                bus_cl_rtu_state = BUS_CL_RTU_WRITE_STREAM;
            } else {
                rs485_write(sizeof(ModbusRtuPacketResponse));
                bus_cl_rtu_state = BUS_CL_RTU_WRITE_RESPONSE_CRC;
            }
        } else {
            // Transmit error data in one go
#define resp_2 ((ModbusRtuPacketErrorResponse*)rs485_buffer)
            resp_2->header.address = bus_cl_stationAddress;
            resp_2->header.function = s_function | 0x80;
            resp_2->error = s_exceptionCode;
            rs485_write(sizeof(ModbusRtuPacketErrorResponse));
            bus_cl_rtu_state = BUS_CL_RTU_WRITE_RESPONSE_CRC;
        }
    }

    if (bus_cl_rtu_state == BUS_CL_RTU_WRITE_STREAM) {
        if (rs485_writeInProgress()) {
            return false;
        }
        uint8_t remaining = s_sizeRemaining;
        // A single call can be done to read the rest of the stream?
        if (s_sizeRemaining > RS485_BUF_SIZE) {
            remaining = RS485_BUF_SIZE;
        }
        (*((s_curRequest.registerAddressH == 0) ? 
            &bus_cl_sysFunctionReadHandlers[s_curRequest.registerAddressL >> 4] : 
            &bus_cl_appFunctionReadHandlers[s_curRequest.registerAddressH - 1]))();
        rs485_write(remaining);
        s_sizeRemaining -= remaining;
        if (s_sizeRemaining > 0) {
            return false;
        } else {
            // Next state
            bus_cl_rtu_state = BUS_CL_RTU_WRITE_RESPONSE_CRC;
        }
    }

    if (bus_cl_rtu_state == BUS_CL_RTU_WRITE_RESPONSE_CRC) {
        if (rs485_writeInProgress()) {
            return false;
        }
        // CRC is LSB first
        *((uint16_t*)rs485_buffer) = htole16(crc16);
        rs485_write(sizeof(uint16_t));
        bus_cl_rtu_state = BUS_CL_RTU_WAIT_FOR_FLUSH;
        return false;
    }

    if (bus_cl_rtu_state == BUS_CL_RTU_WAIT_FOR_FLUSH) {
        if (rs485_state == RS485_LINE_RX) {
            bus_cl_rtu_state = BUS_CL_RTU_IDLE;
        }
    }

    return false;
}
