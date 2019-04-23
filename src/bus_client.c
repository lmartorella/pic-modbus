#include "pch.h"
#include "rs485.h"
#include "hardware/tick.h"
#include "bus.h"
#include "protocol.h"
#include "persistence.h"
#include "appio.h"

#ifdef HAS_BUS_CLIENT

static enum {
    STATE_HEADER_0 = 0,         // header0, 55
    STATE_HEADER_1 = 1,         // header1, aa
    STATE_HEADER_2 = 2,         // header2, address
    STATE_HEADER_ADRESS = 2,         // header2
    STATE_HEADER_3 = 3,         // msgtype
            
    STATE_SOCKET_OPEN = 10,
    STATE_WAIT_TX
            
} s_state;

static BYTE s_header[3] = { 0x55, 0xAA, 0 };
static bit s_availForAddressAssign;
// The master knows me
static bit s_known;
static BYTE s_tempAddressForAssignment;

/**
 * Wait for the channel to be free again and skip the glitch after a TX/RX switch (server DISENGAGE_CHANNEL_TIMEOUT time)
 */
static void bus_reinit_after_disengage()
{
    rs485_waitDisengageTime();
    s_state = STATE_WAIT_TX;
}

/**
 * Quickly return in listen state, without waiting for the disengage time
 */
static void bus_reinit_quick()
{
    s_state = STATE_HEADER_0;
    // Skip bit9 = 0
    rs485_skipData = TRUE;
}

bit bus_isIdle() 
{
    return s_state == STATE_HEADER_0;
}

void bus_init()
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
        pers_data.address = UNASSIGNED_SUB_ADDRESS;       
        pers_save();
        s_availForAddressAssign = TRUE;
    } 

    if (pers_data.address == UNASSIGNED_SUB_ADDRESS) {
        // Signal unattended client, but doesn't auto-assign to avoid line clash at multiple boot
        led_on();
    }

    s_header[2] = pers_data.address;

    bus_reinit_quick();
}

static void bus_storeAddress()
{
    pers_data.address = s_header[2] = s_tempAddressForAssignment;
    pers_save();
    
    s_availForAddressAssign = FALSE;
    led_off();
}

static void bus_sendAck(BYTE ackCode) {
    // Respond with a socket response
    rs485_write(FALSE, s_header, 3);
    rs485_write(FALSE, &ackCode, 1);
    // And then wait for TX end before going idle
    s_state = STATE_WAIT_TX;
}

// Called often
void bus_poll()
{
    BYTE buf;
    
    switch (s_state) {
        case STATE_SOCKET_OPEN: 
            if (rs485_lastRc9) {
                // Received a break char, go idle
#ifdef DEBUGMODE
                printch('@');
#endif
                bus_reinit_after_disengage();
            }
            // Else do nothing
            break;
        case STATE_WAIT_TX:
            // When in read mode again, progress
            if (rs485_state == RS485_LINE_RX) {
                bus_reinit_quick();
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
                    if (s_state == STATE_HEADER_ADRESS && s_availForAddressAssign) {
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
                        bus_reinit_after_disengage();
                    }
                }
                else {
                    // Header correct. Now read the command and respond
                    switch (buf) { 
                        case BUS_MSG_TYPE_ADDRESS_ASSIGN:
#ifdef DEBUGMODE
                            printch('^');
#endif
                            if (s_availForAddressAssign) {
                                // Master now knows me
                                s_known = 1;
                                // Store the new address in memory
                                bus_storeAddress();
                                bus_sendAck(BUS_ACK_TYPE_HEARTBEAT);
                                return;
                            }
                            break;

                        case BUS_MSG_TYPE_HEARTBEAT:
#ifdef DEBUGMODE
                            printch('"');
#endif
                            // Only respond to heartbeat if has address
                            if (s_header[2] != UNASSIGNED_SUB_ADDRESS) {
                                bus_sendAck(s_known ? BUS_ACK_TYPE_HEARTBEAT : BUS_ACK_TYPE_HELLO);
                                return;
                            }
                            break;

                        case BUS_MSG_TYPE_READY_FOR_HELLO:
#ifdef DEBUGMODE
                            printch('?');
#endif
                            // Only respond to hello if ready to program
                            if (s_availForAddressAssign) {
                                bus_sendAck(BUS_ACK_TYPE_HELLO);
                                return;
                            }
                            break;

                        case BUS_MSG_TYPE_CONNECT:
#ifdef DEBUGMODE
                            printch('=');
#endif
                            if (s_header[2] != UNASSIGNED_SUB_ADDRESS) {
                                // Master now knows me
                                s_known = 1;
                                // Start reading data with rc9 not set
                                rs485_skipData = rs485_lastRc9 = FALSE;
                                // Socket, direct connect
                                s_state = STATE_SOCKET_OPEN;
                                return;
                            }
                            break;
                        default:
#ifdef DEBUGMODE
                            printch('!');
#endif
                            // Unknown command. Reset
                            // Restart from 0x55
                            bus_reinit_after_disengage();
                            break;
                    }
                
#ifdef DEBUGMODE
                    printch('-');
#endif
                    // If not managed, reinit bus for the next message
                    bus_reinit_after_disengage();
                }
            }
    }
}

// Close the socket
void prot_control_close()
{
    // set_rs485_close
    rs485_over = 1;
    rs485_close = 1;
    // And then wait for TX end before going idle
    s_state = STATE_WAIT_TX;   
}

// Socket connected?
bit prot_control_isConnected()
{
    return s_state == STATE_SOCKET_OPEN;
}

#endif
