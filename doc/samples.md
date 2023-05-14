# Sample applications

This folder contains some samples of hardware Microchip-based beans to interface the net master with the real world.

## Digital input and output

The most simple sink is the one that let you to access the digital pins of the MCU.

### Output sink

The output sink let the pin values to be changed. High-impedance level is not supported.

This sink doesn't use non-volatile memory: in case of reset, the pin state will reset, and should be reconfigured by the server.

This is the content of the read packet (to read settings):

|Field|Size (bytes)|Description|
|--|--|--|
| Number of bits | 1 | The number of digital pins available |

Once the settings are read, each write command can be used to change the state of the pins:

|Field|Size (bytes)|Description|
|--|--|--|
|Byte count (N)|1|How many bytes will follow. Normally they should be enough to accommodate all the pin levels (e.g. 8 pins in 1 byte)|
|Data|_N_|The packeted values of the pins (LSB pins first)|

### Input sink

The input sink let the pin values to be read. High-impedance level is not supported.

In addition to the current pin values, the log of last unread change event of the level is returned, with the precision of the internal tick counter (<1 ms in the micro-bean). This allow more precision for switches will less polling requirement. Changes are internally polled, no interrupt line is used.

This sink doesn't use non-volatile memory: in case of reset, the pin state will reset, and it should be reconfigured by the firmware.

This is the content of the read packet. It contains static configuration and actual values:

|Field|Size (bytes)|Description|
|--|--|--|
| Tick size + Number of bits | 1 | The byte contains the size in bytes of a tick measure (T) in the low nibble (4 bits), and the number of bits (N) in the high nibble.|
|Last state|_N / 8_|The immediate values of the pins.|
|Tick / seconds|_T_|The number of ticks per seconds.|
|Current time|_T_|The current time in ticks.|
|Event table size|1|The count of change events that follows. If zero, no events will follow.|
|Event #0, time| _T_|The tick value when the change event occurred.|
|Event #0, value| _N / 8_|The value of the pins when the change event occurred.|
|Event #1, time, ...| _T_|[...] _Repeat for every additional event_|

## Pressure and temperature sensor

The code is meant to work with the [Bosch BMP180](https://www.bosch-sensortec.com/bst/products/all_products/bmp180) sensor.

The communication is implemented polling the serial line with the sensor.

The protocol expects a command (1-byte) to be sent, followed by a read operation.

Possible write commands:

|Command (1-byte)|Description|
|--|--|
|COMMAND_RESET_READ_CALIB (0x22)| The command resets the calibration. The next read operation will receive the calibration data (22h bytes will be sent)|
|COMMAND_READ_DATA (0x5)| The next read operation will read the raw data (5 bytes will be sent)|

The raw data and the calibration numbers should be interpreted as the datasheet. See the [server-side routine](../server/Home.Samples/Devices/BarometricTesterDevice.cs) in C# to decode the data.

## Humidity and temperature sensor

This other sink works with the common [DHT11](https://www.mouser.com/datasheet/2/758/DHT11-Technical-Data-Sheet-Translated-Version-1143054.pdf) sensor.

The communication is implemented polling the serial line with the sensor.

The protocol only expects read commands.

The read command returns 6-byte packet:

|Field|Size (bytes)|Description|
|--|--|--|
|Status|1|The sensor status. 1 means OK, other values are errors happened during reading the sensor values. See [here](./sinks/dht11.c) for details.|
|Data|5|The raw data, as documented in the sensor datasheet. See the [server-side routine](../server/Home.Samples/Sinks/TemperatureSink.cs) in C# to decode the data.|

## Flow counter

This sink is used to control the instant flow of water and the total counter of water volume.

This sink uses non-volatile memory to store the total counter. Writes are coalesced to avoid high number of write cycles and to damage the MCU.

Protocol and implementation to be documented.