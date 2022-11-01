#include "net.h"
#include "rs485.h"
#include "timers.h"
#include "bus_secondary.h"
#include "protocol.h"
#include "persistence.h"
#include "appio.h"
#include "leds.h"

/**
 * Wired bus communication module for secondary RS485 nodes
 */

static enum {
    STATE_HEADER_0 = 0,         // header0, 55
    STATE_HEADER_1 = 1,         // header1, aa
    STATE_HEADER_2 = 2,         // header2, address
    STATE_HEADER_ADDRESS = 2,   // header2
    STATE_HEADER_3 = 3,         // msgtype
            
    STATE_SOCKET_OPEN = 10,
    STATE_WAIT_TX
            
} s_state;

static uint8_t s_header[3] = { 0x55, 0xAA, 0 };
static __bit s_availForAddressAssign;
// The master knows me
static __bit s_known;
static uint8_t s_tempAddressForAssignment;

/**
 * Wait for the channel to be free again and skip the glitch after a TX/RX switch (server DISENGAGE_CHANNEL_TIMEOUT time)
 */
static void reinit_after_disengage()
{
    rs485_waitDisengageTime();
    s_state = STATE_WAIT_TX;
}

/**
 * Quickly return in listen state, without waiting for the disengage time
 */
static void reinit_quick()
{
    s_state = STATE_HEADER_0;
    // Skip bit9 = 0
    rs485_skipData = true;
}

void bus_sec_init()
{
    // Prepare address
    s_availForAddressAssign = 0;
    s_known = 0;

    // Address should be reset?
    if (g_resetReason == RESET_MCLR
#ifdef _IS_ETH_CARD
            // Bug of HW?
            || g_resetReason == RESET_POWER
#endif
    ) {
        // Reset address
        pers_data.sec.address = UNASSIGNED_SUB_ADDRESS;       
        pers_save();
        s_availForAddressAssign = true;
    } 

    if (pers_data.sec.address == UNASSIGNED_SUB_ADDRESS) {
        // Signal unattended secondary client, but doesn't auto-assign to avoid line clash at multiple boot
        led_on();
    }

    s_header[2] = pers_data.sec.address;

    reinit_quick();
}

static void storeAddress()
{
    pers_data.sec.address = s_header[2] = s_tempAddressForAssignment;
    pers_save();
    
    s_availForAddressAssign = false;
    led_off();
}

static void sendAck(uint8_t ackCode) {
    // Respond with a socket response
    rs485_write(false, s_header, 3);
    rs485_write(false, &ackCode, 1);
    // And then wait for TX end before going idle
    s_state = STATE_WAIT_TX;
}

// Called often
__bit bus_sec_poll()
{
    uint8_t buf;

    switch (s_state) {
        case STATE_SOCKET_OPEN: 
            if (rs485_lastRc9) {
                // Received a break char, go idle
#ifdef DEBUGMODE
                io_printChDbg('@');
#endif
                reinit_after_disengage();
            }
            // Else do nothing
            break;
        case STATE_WAIT_TX:
            // When in read mode again, progress
            if (rs485_state == RS485_LINE_RX) {
                reinit_quick();
            }
            break;
        default:
            // Header decode
            if (rs485_readAvail() > 0) {            

                // RC9 will be 1
                rs485_read(&buf, 1);
                // Waiting for header?
                if (s_state < STATE_HEADER_3) {
                    // Keep an eye to address if in assign state
                    if (s_state == STATE_HEADER_ADDRESS && s_availForAddressAssign) {
                        // Store the byte, in case of s_skipNextAddressCheck
                        s_tempAddressForAssignment = buf;
                        // Go ahead
                        s_state++;
                    }
                    else if (buf == s_header[s_state]) {
                        // Header matches. Go next.
                        s_state++;
                    }
                    else {
                        // Not my address, or protocol error. Restart from 0x55 but don't try to skip glitches
                        reinit_after_disengage();
                    }
                }
                else {
                    _Bool isManaged = false;
                    // Header correct. Now read the command and respond
                    switch (buf) { 
                        case BUS_MSG_TYPE_ADDRESS_ASSIGN:
#ifdef DEBUGMODE
                            io_printChDbg('^');
#endif
                            if (s_availForAddressAssign) {
                                // Master now knows me
                                s_known = 1;
                                // Store the new address in memory
                                storeAddress();
                                sendAck(BUS_ACK_TYPE_HEARTBEAT);
                                isManaged = true;
                            }
                            break;

                        case BUS_MSG_TYPE_HEARTBEAT:
#ifdef DEBUGMODE
                            io_printChDbg('"');
#endif
                            // Only respond to heartbeat if has address
                            if (s_header[2] != UNASSIGNED_SUB_ADDRESS) {
                                sendAck(s_known ? 
                                    (g_resetReason == RESET_NONE ? BUS_ACK_TYPE_HEARTBEAT : BUS_ACK_TYPE_READ_STATUS) 
                                        : BUS_ACK_TYPE_HELLO);
                                isManaged = true;
                            }
                            break;

                        case BUS_MSG_TYPE_READY_FOR_HELLO:
#ifdef DEBUGMODE
                            io_printChDbg('?');
#endif
                            // Only respond to hello if ready to program
                            if (s_availForAddressAssign) {
                                sendAck(BUS_ACK_TYPE_HELLO);
                                isManaged = true;
                            }
                            break;

                        case BUS_MSG_TYPE_CONNECT:
#ifdef DEBUGMODE
                            io_printChDbg('=');
#endif
                            if (s_header[2] != UNASSIGNED_SUB_ADDRESS) {
                                // Master now knows me
                                s_known = 1;
                                // Start reading data with rc9 not set
                                rs485_skipData = rs485_lastRc9 = false;
                                // Socket, direct connect
                                s_state = STATE_SOCKET_OPEN;
                                isManaged = true;
                            }
                            break;
                        default:
#ifdef DEBUGMODE
                            io_printChDbg('!');
#endif
                            // Unknown command. Reset
                            // Restart from 0x55
                            reinit_after_disengage();
                            break;
                    }
                
#ifdef DEBUGMODE
                    io_printChDbg('-');
#endif
                    if (!isManaged) {
                        // If not managed, reinit bus for the next message
                        reinit_after_disengage();
                    }
                }
            }
    }

    return s_state != STATE_HEADER_0;
}

// Close the socket
void bus_sec_abort() {
    // set_rs485_close
    rs485_over = 1;
    rs485_close = 1;
    // And then wait for TX end before going idle
    s_state = STATE_WAIT_TX;   
}

// Socket connected?
__bit bus_sec_isConnected()
{
    return s_state == STATE_SOCKET_OPEN;
}
