// This file is requested by TCP/IP stack

// For Tcp/ip stack
#define GetSystemClock()    (SYSTEM_CLOCK)
#define GetInstructionClock() (SYSTEM_CLOCK/4)
#define GetPeripheralClock() (SYSTEM_CLOCK/4)

#include <TCPIPStack/ETH97J60.h>
