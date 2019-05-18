# Microbean prototype

The prototyped hardware is a small board (just 30x28mm) built around a cost-effective [Microchip PIC16F1827 MCU](https://www.microchip.com/wwwproducts/en/PIC16F1827), a RS-485 level adapter and a voltage regulator. The board expose almost all MCU lines to the maker, allowing a great amount of free digital and analog I/O pins.

![The microbean](./ubean.png)

Once the MCU are programmed, the network topology can be easily configured with a PC.

Each physical node exposes one or more applicative drivers, called *sinks*. In this way, a single bean can be programmed to interface more than one device.

# Schematic

The schematics is oriented to simplicity and stability.

![Schematic](./microbean.png)

The voltage regulation is delegated to a classic 78 series regulator, to be compatible with a wide range of line input voltage. A serie diode saves the boards from inverted wiring. The electrolytics capacitors are oversized to comply with electric surges.

Since the regulated voltage is exposed through the output ports to power any attached hardware (sensors, small displays, etc...), the regulator should be dimensioned to provide the required current (depending also from the voltage swing of the power line).

Each boards can be fitted with 5V or 3.3V regulator, without any issue for the Mcirochip MCU that supports a wide input voltage. This enables a wide range of sensors to be directly connected to the MCU ports without requiring any level adapter.

A full-sized In-Circuit Serial Programmer for the MCU port is provided, with classic 0.1" pitch, to provide direct programming feature.

The RS485 bus adapter IC should be fitted based on the voltage regulator. Both voltage flavours are available from ST (ST485/ST3485).

The rest of the small board is occupied by the micro-switch, the status LED, external connectors and the 4-wire screw-type terminals.

# The Microcontroller

The selected MCU is the [PIC16L1827](https://www.microchip.com/wwwproducts/en/PIC16F1827) from Microchip, a mid-range 8-bit CPU with 4K program memory that gives a great selection of features in a small SO18 package that can be still soldered without requiring specialized tools.

In addition, pins can be configured with some degree of freedom in order to expose the hardware features as required.   

The following MCU lines are used to connect the bean to the RS485 bus, hence are not available for design:

- RB2 as UART RX
- RB5 as UART TX
- RA6 for  RS485 arbitration
- RA5/MCLR for zero-configuration button
- RA7 for status led

In addition these lines are shared with ICSP:
- RB6 with ICSP CLK
- RB7 with ICSP DAT

The configuration leaves a total of 11 ports to the designer.


