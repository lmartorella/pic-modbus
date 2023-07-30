#include "pic-modbus/rtu_client.h"
#include "pic-modbus/crc.h"

#if defined _CONF_RS485

#include "pic-modbus/rs485.h"
#define pkt_read_avail rs485_read_avail
#define pkt_buffer rs485_buffer
#define pkt_write rs485_write
#define pkt_write_in_progress rs485_write_in_progress
#define pkt_write_end()
#define pkt_discard rs485_discard
#define pkt_packet_end rs485_packet_end
#define pkt_in_receive_state rs485_in_receive_state
#define pkt_init rs485_init
#define pkt_poll rs485_poll

#elif defined _CONF_PACKET_RADIO

#include "pic-modbus/radio.h"
#define pkt_read_avail radio_read_avail
#define pkt_buffer radio_buffer
#define pkt_write radio_write
#define pkt_write_in_progress radio_write_in_progress
#define pkt_write_end radio_write_end
#define pkt_discard radio_discard
#define pkt_packet_end radio_packet_end
#define pkt_in_receive_state radio_in_receive_state
#define pkt_init radio_init
#define pkt_poll radio_poll

#else
#error You should define _CONF_RS485 or _CONF_PACKET_RADIO to select the channel medium
#endif

#ifdef _CONF_RS485
// On the RS485 RTU, the CRC is part of the protocol. Packet radio instead
// is supposed to have his own CRC
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
    pkt_init();
}

// Called often
_Bool rtu_cl_poll() {
    _Bool active = pkt_poll();
    if (pkt_packet_end && rtu_cl_state != RTU_CL_IDLE) {
        if (rtu_cl_state != RTU_CL_WAIT_FOR_RESPONSE) {
            // Abort reading, go idle
            rtu_cl_state = RTU_CL_IDLE;
            return active;
        } else {
            rtu_cl_state = RTU_CL_RESPONSE;
        }
    }

    if (rtu_cl_state == RTU_CL_IDLE) {
        // Wait for at least a read message request size
        if (pkt_read_avail() < sizeof(ModbusRtuHoldingRegisterRequest)) {
            // Nothing to do, wait for more data
            return active;
        }
        // Read and free the buffer
        rtu_cl_header = *((const ModbusRtuHoldingRegisterRequest*)pkt_buffer);
        pkt_discard(sizeof(ModbusRtuHoldingRegisterRequest));

        if (rtu_cl_header.header.stationAddress == STATION_NODE) {
            if (rtu_cl_header.header.function == READ_HOLDING_REGISTERS || rtu_cl_header.header.function == WRITE_HOLDING_REGISTERS) {
                if (!regs_validateReg()) {
                    // Error was set, respond with error
                    rtu_cl_state = RTU_CL_WAIT_FOR_RESPONSE;
                    return active;
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
                return active;
            }
        } else {
            // No this station, wait for idle
            rtu_cl_state = RTU_CL_WAIT_FOR_IDLE;
        }
    }
    
    if (rtu_cl_state == RTU_CL_RECEIVE_DATA_SIZE) {
        if (pkt_read_avail() < 1) {
            // Nothing to do, wait for more data
            return active;
        }
        // Free the buffer
        pkt_discard(1);
        if (pkt_buffer[0] != messageSize) {
            // Invalid size, return error
            rtu_cl_exceptionCode = ERR_INVALID_SIZE;
            rtu_cl_state = RTU_CL_WAIT_FOR_RESPONSE;
            return active;
        } else {
            rtu_cl_state = RTU_CL_RECEIVE_DATA;
        }
    }

    if (rtu_cl_state == RTU_CL_RECEIVE_DATA) {
        // Wait for register data + count byte
        if (pkt_read_avail() < messageSize) {
            // Nothing to do, wait for more data
            return active;
        }
        // Free the buffer
        pkt_discard(messageSize);
        if (!regs_onReceive()) {
            // Data/custom error, error is set
            rtu_cl_state = RTU_CL_WAIT_FOR_RESPONSE;
            return active;
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

        if (pkt_read_avail() < sizeof(uint16_t)) {
            // Nothing to do, wait for more data
            return active;
        }
        // Free the buffer
        pkt_discard(sizeof(uint16_t));

        rtu_cl_state = RTU_CL_WAIT_FOR_RESPONSE;
        if (expectedCrc != *((const uint16_t*)pkt_buffer)) {
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
        pkt_discard(pkt_read_avail());
        return active;
    }

    if (rtu_cl_state == RTU_CL_RESPONSE) {
        // Copy whole header (and overwrite data in case of error)
        // Response of write registers always contains the address and register count
        *((ModbusRtuHoldingRegisterRequest*)pkt_buffer) = rtu_cl_header;
        if (rtu_cl_exceptionCode == NO_ERROR) {
            // Transmit packet data in one go
            if (rtu_cl_header.header.function == READ_HOLDING_REGISTERS) {
                // Now, if write, open stream
                ((ModbusRtuPacketReadResponse*)pkt_buffer)->size = messageSize;
                pkt_write(sizeof(ModbusRtuPacketReadResponse));
                rtu_cl_state = RTU_CL_SEND_DATA;
            } else {
                pkt_write(sizeof(ModbusRtuPacketWriteResponse));
#ifdef USE_CRC
                rtu_cl_state = RTU_CL_WRITE_RESPONSE_CRC;
#else
                pkt_write_end();
                rtu_cl_state = RTU_CL_WAIT_FOR_FLUSH;
#endif
            }
        } else {
            // Transmit error data in one go
            ((ModbusRtuPacketErrorResponse*)pkt_buffer)->header.function = ((ModbusRtuPacketErrorResponse*)pkt_buffer)->header.function | 0x80;
            ((ModbusRtuPacketErrorResponse*)pkt_buffer)->error = rtu_cl_exceptionCode;
            pkt_write(sizeof(ModbusRtuPacketErrorResponse));
#ifdef USE_CRC
            rtu_cl_state = RTU_CL_WRITE_RESPONSE_CRC;
#else
            pkt_write_end();
            rtu_cl_state = RTU_CL_WAIT_FOR_FLUSH;
#endif
        }
    }

    if (rtu_cl_state == RTU_CL_SEND_DATA) {
        // Wait for the bus to switch over
        if (pkt_write_in_progress()) {
            return active;
        }
        regs_onSend();
        pkt_write(messageSize);
#ifdef USE_CRC
        rtu_cl_state = RTU_CL_WRITE_RESPONSE_CRC;
#else
        pkt_write_end();
        rtu_cl_state = RTU_CL_WAIT_FOR_FLUSH;
#endif
    }

#ifdef USE_CRC
    if (rtu_cl_state == RTU_CL_WRITE_RESPONSE_CRC) {
        if (pkt_write_in_progress()) {
            return active;
        }
        // CRC is LSB first
        *((uint16_t*)pkt_buffer) = htole16(crc16);
        pkt_write(sizeof(uint16_t));
        pkt_write_end();
        rtu_cl_state = RTU_CL_WAIT_FOR_FLUSH;
        return active;
    }
#endif
    
    if (rtu_cl_state == RTU_CL_WAIT_FOR_FLUSH) {
        if (pkt_in_receive_state()) {
            rtu_cl_state = RTU_CL_IDLE;
        }
    }

    return active;
}
