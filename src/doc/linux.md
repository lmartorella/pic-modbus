# Testing dongle for RS485 RTU

Use Pl2303 USB to Serial adapter, with a RS485 level adapter built-in.

Find it via

```sh
dmesg | grep tty
```

(usually `ttyUSB0`).

Trying [Modbus CLI](https://github.com/favalex/modbus-cli)

To access the PIC RTU line, that is configured with 19200 baud, 8,N,1, the command line is

```sh
modbus --registers=$FILE  --baud=19200 --stop-bits=1 --parity=n --byte-order=be ttyUSB0
```

where `$FILE` is for example:

- `H@0/h` to read the 16-bytes 