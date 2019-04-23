#include "../pch.h"
#include "spiram.h"
#include "../protocol.h"
#include "../appio.h"

#ifdef FLASHER_APP

static void createFlasherSink(void);
static void destroyFlasherSink(void);
static void pollFlasherSink(void);

#define MSG_FLASH_ROM 0

#define SINK_FLASHER_PORT (SINK_FLASHER_TYPE + BASE_SINK_PORT)
const Sink g_flasherSink = { 
                             SINK_FLASHER_TYPE,
                             0, 
                             SINK_FLASHER_PORT,
                             &createFlasherSink,
                             &destroyFlasherSink,
                             &pollFlasherSink };

// The TCP client socket of display listener
static TCP_SOCKET s_listenerSocket = INVALID_SOCKET;

static void createFlasherSink()
{
	// Open the sever TCP channel
	s_listenerSocket = TCPOpen(0, TCP_OPEN_SERVER, SINK_FLASHER_PORT, TCP_PURPOSE_GENERIC_TCP_SERVER);
	if (s_listenerSocket == INVALID_SOCKET)
	{
		fatal("FLH_SRV");
	}
}

static void destroyFlasherSink()
{
	if (s_listenerSocket != INVALID_SOCKET)
	{
		TCPClose(s_listenerSocket);
	}
}

static void pollFlasherSink()
{
	static DWORD ptr = 0xffffffff;

	unsigned short s;
	if (!TCPIsConnected(s_listenerSocket))
	{
		return;
	}

loop:
	s = TCPIsGetReady(s_listenerSocket);
	if (s > sizeof(WORD))
	{
		if (ptr == 0xffffffff)
		{	
			// Not started. Read the msg code
			WORD msg;
			TCPGetArray(s_listenerSocket, (BYTE*)&msg, sizeof(WORD));
			if (msg != MSG_FLASH_ROM)
			{
				fatal("FLH_MSG");
			}
			// Enter in flash ROM mode
			ptr = 0;
			goto loop;
		}
		else
		{
			// Write to RAM!
			for (; s > 0; s--)
			{
				BYTE b;
				if (!TCPGet(s_listenerSocket, &b))
				{
					fatal("FLH_RD");
				}
				sram_write(&b, ptr, 1);
				ptr++;

				//<EXIT CONDITION> when 128kb is reached
                                // Optimize loop! Use buffer
                                fatal("OKEND");
			}
		}
	}
}

#endif