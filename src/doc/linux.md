# Testing dongle for RS485 RTU

Use PL2303 USB to Serial adapter, with a RS485 level adapter built-in.

```sh
dmesg | grep tty
```

(usually `ttyUSB0`).

> The original dongle uses an automatic timered switch between RX and TX. The PL2303 however exposes the RTS line at the pin 3 (negated), and it can be used to drive both the `DE` and `/RE` MAX485 lines. This means that an "unasserted" RTS drives the transmit line, and a "asserted" (up) RTS will enable the receive line. This mode is called `MODBUS_RTU_RTS_DOWN` in the `libmodbus` API.

> The `libmodbus` tries to use the `TIOCGRS485` if the backend mode is set to `RTU`. This is not supported by PL2303.

# mbpoll

The `mbpoll` app requires `-R` to use the `MODBUS_RTU_RTS_DOWN` mode.

> Note: the `RTU` mode

```sh
mbpoll -m RTU -a 1 -r 16 -c 8 -t 4:hex -1 -b 19200 -d 8 -s 1 -P none -0 -R /dev/ttyUSB0
```

Still to check: -R or -F

# Modbus CLI

Trying [Modbus CLI](https://github.com/favalex/modbus-cli)

To access the PIC RTU line, that is configured with 19200 baud, 8,N,1, the command line is

```sh
modbus --registers=$FILE  --baud=19200 --stop-bits=1 --parity=n --byte-order=be ttyUSB0
```

where `$FILE` is for example:

- `H@0/h` to read the 16-bytes 