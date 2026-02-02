# Modbus for PIC16 Microchip microcontroller

This is a ANSI-C portable, small-footprint, serial-line (RTU) client implementation of [Modbus](https://en.wikipedia.org/wiki/Modbus) for the [8-bit PIC16 family](https://www.microchip.com/en-us/products/microcontrollers-and-microprocessors/8-bit-mcus/pic-mcus) of Microchip microcontrollers, with application limited to holding registries reads/writes.

On a [PIC16F1827](https://www.microchip.com/en-us/product/PIC16F1827) device, (with ~7Kb of program memory and 384 bytes of data memory), the library built with the free version of Microchip XC8 compiler, takes roughly 1/4 of the total resources.

![](./doc/mem.png)

## Features

- Implements Modbus functions 0x3 and 0x10 (Read/Write Holding Registries) via custom handlers
- Minimal memory, code and stack footprint to cope with lowest specs 8-bit MCUs
- Direct connection to MAX485/MAX3485 level translator, via RX/TX/OE pins
- Leverage hardware USART and timers for accurate protocol timing
- Implement system register (check reset/error codes, CRC errors, etc...)
- Optimized for free XC8 compiler
- Uses three I/O pins (a pair of UART TX/RX a one for RS485 direction) and one timer with hardware interrupt support (by default the TMR0 module)

## Samples

In addition to the bare library, the folder `samples` contains some sample code for simple applications.

More details can be found [here](./samples).

## A small footprint hardware implementation

My current prototype is a small board, made to fit in wall switches, with power regulator (3.3V or 5V), RS-485 adapter, and a micro-switch and LED.
It sports a `PIC16F1827` with nine general-purpose I/O ports and bus/supply pins available through a 0.1” header. 

More details can be found [here](./microbean).
