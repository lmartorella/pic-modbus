#include "pic-modbus/bus_client.h"
#include "pic-modbus/crc.h"
#include "pic-modbus/rs485.h"

/**
 * Specific module for client Modbus RTU nodes (RS485), optimized
 * for 8-bit CPUs with low memory.
 */

ModbusRtuHoldingRegisterRequest bus_cl_header;

typedef struct {
    ModbusRtuPacketHeader header;
    uint8_t error;
} ModbusRtuPacketErrorResponse;

// Write response is the same of the request header
typedef ModbusRtuHoldingRegisterRequest ModbusRtuPacketWriteResponse;

typedef struct {
    ModbusRtuPacketHeader header;
    uint8_t size;
} ModbusRtuPacketReadResponse;

// If != NO_ERR, write an error
uint8_t bus_cl_exceptionCode;
static uint8_t messageSize;

BUS_CL_RTU_STATE bus_cl_rtu_state;
uint8_t bus_cl_crcErrors;

void bus_cl_init() {
    // RS485 already in receive mode
    bus_cl_rtu_state = BUS_CL_RTU_IDLE;
    bus_cl_crcErrors = 0;
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
        // Wait for at least a read message request size
        if (rs485_readAvail() < sizeof(ModbusRtuHoldingRegisterRequest)) {
            // Nothing to do, wait for more data
            return false;
        }
        // Read and free the buffer
        bus_cl_header = *((const ModbusRtuHoldingRegisterRequest*)rs485_buffer);
        rs485_discard(sizeof(ModbusRtuHoldingRegisterRequest));

        if (bus_cl_header.header.stationAddress == STATION_NODE) {
            if (bus_cl_header.header.function == READ_HOLDING_REGISTERS || bus_cl_header.header.function == WRITE_HOLDING_REGISTERS) {
                if (!regs_validateAddr()) {
                    // Error was set, respond with error
                    bus_cl_rtu_state = BUS_CL_RTU_WAIT_FOR_RESPONSE;
                    return false;
                }
                // Count(16) is always < 128
                messageSize = ((uint8_t)bus_cl_header.address.countL) * 2;

                if (bus_cl_header.header.function == READ_HOLDING_REGISTERS) {
                    // Ok, function data must be read. Wait for packet to end with CRC and then send
                    // response
                    bus_cl_rtu_state = BUS_CL_RTU_CHECK_REQUEST_CRC;
                } else {
                    bus_cl_rtu_state = BUS_CL_RTU_RECEIVE_DATA_SIZE;
                }
            } else {
                // Invalid function, return error
                bus_cl_exceptionCode = ERR_INVALID_FUNCTION;
                bus_cl_rtu_state = BUS_CL_RTU_WAIT_FOR_RESPONSE;
                return false;
            }
        } else {
            // No this station, wait for idle
            bus_cl_rtu_state = BUS_CL_RTU_WAIT_FOR_IDLE;
        }
    }
    
    if (bus_cl_rtu_state == BUS_CL_RTU_RECEIVE_DATA_SIZE) {
        if (rs485_readAvail() < 1) {
            // Nothing to do, wait for more data
            return false;
        }
        if (rs485_buffer[0] != messageSize) {
            // Invalid size, return error
            bus_cl_exceptionCode = ERR_INVALID_SIZE;
            bus_cl_rtu_state = BUS_CL_RTU_WAIT_FOR_RESPONSE;
            return false;
        } else {
            bus_cl_rtu_state = BUS_CL_RTU_RECEIVE_DATA;
        }
    }

    if (bus_cl_rtu_state == BUS_CL_RTU_RECEIVE_DATA) {
        // Wait for register data + count byte
        if (rs485_readAvail() < messageSize) {
            // Nothing to do, wait for more data
            return false;
        }
        // Free the buffer
        rs485_discard(messageSize);
        if (!regs_onReceive()) {
            // Data/custom error, error is set
            bus_cl_rtu_state = BUS_CL_RTU_WAIT_FOR_RESPONSE;
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
            // TODO: However the function data was already sent to registers!
            bus_cl_crcErrors++;
            bus_cl_rtu_state = BUS_CL_RTU_WAIT_FOR_IDLE;
        } else {
            // Ok, go on with the response
            bus_cl_exceptionCode = NO_ERROR;
        }
    }

    if (bus_cl_rtu_state == BUS_CL_RTU_WAIT_FOR_IDLE) {
        rs485_discard(rs485_readAvail());
        return false;
    }

    if (bus_cl_rtu_state == BUS_CL_RTU_RESPONSE) {
        // Copy whole header (and overwrite data in case of error)
        // Response of write registers always contains the address and register count
        *((ModbusRtuHoldingRegisterRequest*)rs485_buffer) = bus_cl_header;
        if (bus_cl_exceptionCode == NO_ERROR) {
            // Transmit packet data in one go
            if (bus_cl_header.header.function == READ_HOLDING_REGISTERS) {
                // Now, if write, open stream
                ((ModbusRtuPacketReadResponse*)rs485_buffer)->size = messageSize;
                rs485_write(sizeof(ModbusRtuPacketReadResponse));
                bus_cl_rtu_state = BUS_CL_RTU_SEND_DATA;
            } else {
                rs485_write(sizeof(ModbusRtuPacketWriteResponse));
                bus_cl_rtu_state = BUS_CL_RTU_WRITE_RESPONSE_CRC;
            }
        } else {
            // Transmit error data in one go
            ((ModbusRtuPacketErrorResponse*)rs485_buffer)->header.function = ((ModbusRtuPacketErrorResponse*)rs485_buffer)->header.function | 0x80;
            ((ModbusRtuPacketErrorResponse*)rs485_buffer)->error = bus_cl_exceptionCode;
            rs485_write(sizeof(ModbusRtuPacketErrorResponse));
            bus_cl_rtu_state = BUS_CL_RTU_WRITE_RESPONSE_CRC;
        }
    }

    if (bus_cl_rtu_state == BUS_CL_RTU_SEND_DATA) {
        // Wait for the bus to switch over
        if (rs485_writeInProgress()) {
            return false;
        }
        regs_onSend();
        rs485_write(messageSize);
        bus_cl_rtu_state = BUS_CL_RTU_WRITE_RESPONSE_CRC;
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
