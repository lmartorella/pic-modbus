#ifndef RS485_H
#define	RS485_H

/**
 * High-level generic RS485 operation module virtualization. 
 * It basically encapsulate a 8-bit UART with additional direction bit (usually RTS on RS485 USB dongles).
 */

/**
 * Initialize asynchronous mode, but only half-duplex is used
 */
void rs485_init(void);

/**
 * When the bus is engaged and a packet is transiting, poll should be called with 
 * a frequency at least 0.5 character period (e.g. 280us on 19200 baud).
 * When the bus is idle instead, the poll can be less intensive, depending 
 * on the HW buffer size of the UART in use (e.g. 2 characters, so 1ms on 19200 baud, on a PIC MCU, or even 
 * slower on a more buffered UART, but at least able to detect a message before his end).
 * Returns `true` if the bus is active (so fast poll is required).
 */
_Bool rs485_poll(void);

/**
 * Enqueue bytes to send. Buffer is copied so it is safe to be reused.
 */ 
void rs485_write(const uint8_t* data, uint8_t size);

/**
 * Read data, if available.
 */
__bit rs485_read(uint8_t* data, uint8_t size);

/**
 * Get count of available bytes in the read buffer
 */
uint8_t rs485_readAvail();

/**
 * Get count of available bytes in the write buffer
 */
uint8_t rs485_writeAvail(void);

#endif	/* RS485_H */

