#include "pic-modbus/rtu_client.h"
#include "pic-modbus/crc.h"

#if defined _CONF_RS485
#include "pic-modbus/rs485.h"
#elif defined _CONF_RADIO
#include "pic-modbus/radio.h"
#else
#error You should define _CONF_RS485 or _CONF_RADIO to select the channel medium
#endif

#ifdef _CONF_RS485
#define USE_CRC
#endif

/**
 * Specific module for client Modbus RTU nodes (RS485 or radio), optimized
 * for 8-bit CPUs with low memory.
 */

ModbusRtuHoldingRegisterRequest rtu_cl_header;

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
uint8_t rtu_cl_exceptionCode;
static uint8_t messageSize;

RTU_CL_STATE rtu_cl_state;

#ifdef USE_CRC
uint8_t rtu_cl_crcErrors;
#endif

void rtu_cl_init() {
    // RS485/radio already considered in receive mode
    rtu_cl_state = RTU_CL_IDLE;
#ifdef USE_CRC
    rtu_cl_crcErrors = 0;
#endif
}

// Called often
__bit rtu_cl_poll() {
    if (rs485_isMarkCondition && rtu_cl_state != RTU_CL_IDLE) {
        if (rtu_cl_state != RTU_CL_WAIT_FOR_RESPONSE) {
            // Abort reading, go idle
            rtu_cl_state = RTU_CL_IDLE;
            return false;
        } else {
            rtu_cl_state = RTU_CL_RESPONSE;
        }
    }

    if (rtu_cl_state == RTU_CL_IDLE) {
        // Wait for at least a read message request size
        if (rs485_readAvail() < sizeof(ModbusRtuHoldingRegisterRequest)) {
            // Nothing to do, wait for more data
            return false;
        }
        // Read and free the buffer
        rtu_cl_header = *((const ModbusRtuHoldingRegisterRequest*)rs485_buffer);
        rs485_discard(sizeof(ModbusRtuHoldingRegisterRequest));

        if (rtu_cl_header.header.stationAddress == STATION_NODE) {
            if (rtu_cl_header.header.function == READ_HOLDING_REGISTERS || rtu_cl_header.header.function == WRITE_HOLDING_REGISTERS) {
                if (!regs_validateReg()) {
                    // Error was set, respond with error
                    rtu_cl_state = RTU_CL_WAIT_FOR_RESPONSE;
                    return false;
                }
                // Count(16) is always < 128
                messageSize = ((uint8_t)rtu_cl_header.address.countL) * 2;

                if (rtu_cl_header.header.function == READ_HOLDING_REGISTERS) {
                    // Ok, function data must be read. Wait for packet to end with CRC and then send
                    // response
#ifdef USE_CRC
                    rtu_cl_state = RTU_CL_CHECK_REQUEST_CRC;
#else
                    rtu_cl_state = RTU_CL_WAIT_FOR_RESPONSE;
#endif
                } else {
                    rtu_cl_state = RTU_CL_RECEIVE_DATA_SIZE;
                }
            } else {
                // Invalid function, return error
                rtu_cl_exceptionCode = ERR_INVALID_FUNCTION;
                rtu_cl_state = RTU_CL_WAIT_FOR_RESPONSE;
                return false;
            }
        } else {
            // No this station, wait for idle
            rtu_cl_state = RTU_CL_WAIT_FOR_IDLE;
        }
    }
    
    if (rtu_cl_state == RTU_CL_RECEIVE_DATA_SIZE) {
        if (rs485_readAvail() < 1) {
            // Nothing to do, wait for more data
            return false;
        }
        // Free the buffer
        rs485_discard(1);
        if (rs485_buffer[0] != messageSize) {
            // Invalid size, return error
            rtu_cl_exceptionCode = ERR_INVALID_SIZE;
            rtu_cl_state = RTU_CL_WAIT_FOR_RESPONSE;
            return false;
        } else {
            rtu_cl_state = RTU_CL_RECEIVE_DATA;
        }
    }

    if (rtu_cl_state == RTU_CL_RECEIVE_DATA) {
        // Wait for register data + count byte
        if (rs485_readAvail() < messageSize) {
            // Nothing to do, wait for more data
            return false;
        }
        // Free the buffer
        rs485_discard(messageSize);
        if (!regs_onReceive()) {
            // Data/custom error, error is set
            rtu_cl_state = RTU_CL_WAIT_FOR_RESPONSE;
            return false;
        } else {
            // Next state
#ifdef USE_CRC
            rtu_cl_state = RTU_CL_CHECK_REQUEST_CRC;
#else
            rtu_cl_state = RTU_CL_WAIT_FOR_RESPONSE;
#endif
        }
    }

#ifdef USE_CRC
    if (rtu_cl_state == RTU_CL_CHECK_REQUEST_CRC) {
        // CRC is LSB first
        uint16_t expectedCrc = le16toh(crc16);

        if (rs485_readAvail() < sizeof(uint16_t)) {
            // Nothing to do, wait for more data
            return false;
        }
        // Free the buffer
        rs485_discard(sizeof(uint16_t));

        rtu_cl_state = RTU_CL_WAIT_FOR_RESPONSE;
        if (expectedCrc != *((const uint16_t*)rs485_buffer)) {
            // Invalid CRC, skip data.
            // TODO: However the function data was already sent to registers!
            rtu_cl_crcErrors++;
            rtu_cl_state = RTU_CL_WAIT_FOR_IDLE;
        } else {
            // Ok, go on with the response
            rtu_cl_exceptionCode = NO_ERROR;
        }
    }
#endif
    
    if (rtu_cl_state == RTU_CL_WAIT_FOR_IDLE) {
        rs485_discard(rs485_readAvail());
        return false;
    }

    if (rtu_cl_state == RTU_CL_RESPONSE) {
        // Copy whole header (and overwrite data in case of error)
        // Response of write registers always contains the address and register count
        *((ModbusRtuHoldingRegisterRequest*)rs485_buffer) = rtu_cl_header;
        if (rtu_cl_exceptionCode == NO_ERROR) {
            // Transmit packet data in one go
            if (rtu_cl_header.header.function == READ_HOLDING_REGISTERS) {
                // Now, if write, open stream
                ((ModbusRtuPacketReadResponse*)rs485_buffer)->size = messageSize;
                rs485_write(sizeof(ModbusRtuPacketReadResponse));
                rtu_cl_state = RTU_CL_SEND_DATA;
            } else {
                rs485_write(sizeof(ModbusRtuPacketWriteResponse));
#ifdef USE_CRC
                rtu_cl_state = RTU_CL_WRITE_RESPONSE_CRC;
#else
                rtu_cl_state = RTU_CL_WAIT_FOR_FLUSH;
#endif
            }
        } else {
            // Transmit error data in one go
            ((ModbusRtuPacketErrorResponse*)rs485_buffer)->header.function = ((ModbusRtuPacketErrorResponse*)rs485_buffer)->header.function | 0x80;
            ((ModbusRtuPacketErrorResponse*)rs485_buffer)->error = rtu_cl_exceptionCode;
            rs485_write(sizeof(ModbusRtuPacketErrorResponse));
#ifdef USE_CRC
            rtu_cl_state = RTU_CL_WRITE_RESPONSE_CRC;
#else
            rtu_cl_state = RTU_CL_WAIT_FOR_FLUSH;
#endif
        }
    }

    if (rtu_cl_state == RTU_CL_SEND_DATA) {
        // Wait for the bus to switch over
        if (rs485_writeInProgress()) {
            return false;
        }
        regs_onSend();
        rs485_write(messageSize);
#ifdef USE_CRC
        rtu_cl_state = RTU_CL_WRITE_RESPONSE_CRC;
#else
        rtu_cl_state = RTU_CL_WAIT_FOR_FLUSH;
#endif
    }

#ifdef USE_CRC
    if (rtu_cl_state == RTU_CL_WRITE_RESPONSE_CRC) {
        if (rs485_writeInProgress()) {
            return false;
        }
        // CRC is LSB first
        *((uint16_t*)rs485_buffer) = htole16(crc16);
        rs485_write(sizeof(uint16_t));
        rtu_cl_state = RTU_CL_WAIT_FOR_FLUSH;
        return false;
    }
#endif
    
    if (rtu_cl_state == RTU_CL_WAIT_FOR_FLUSH) {
        if (rs485_state == RS485_LINE_RX) {
            rtu_cl_state = RTU_CL_IDLE;
        }
    }

    return false;
}
