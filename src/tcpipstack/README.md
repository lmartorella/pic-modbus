This folders contains the Microchip redistributable implementation of a full working TCP/IP stack.

This library is only used for net master implemented with the PIC18F87J60 MCU.

Only the required source files are compiled by the project:

- ARP.c, DHCP.c, ICMP.c (for DHCP client and ping feature)
- ETH97J60.c (to support the MCU)
- IP.c, UDP.c, TCP.c for the required IP stack.

In order to have a reduced memory footprint.
