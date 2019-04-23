#include "../pch.h"
#include "audioSink.h"
#include "../appio.h"

#ifdef HAS_VS1011

static void createAudioSink(void);
static void destroyAudioSink(void);
static void pollAudioSink(void);

// Only uses max 64Kb for memory. Uses 16bit pointers
/*static*/ WORD _ringStart, _ringEnd;
#define RING_SIZE 0x8000
#define RING_MASK 0x7FFF
#define RING_BUFFER_START_THRESHOLD 0x7000
#define MEM_OFFSET 0

// Range from 0 to (RING_SIZE - 1)
inline static WORD queueSize()
{
    //return ((_ringEnd - _ringStart) + RING_SIZE) % RING_SIZE;
    return (_ringEnd - _ringStart) & (RING_MASK);
}

// Range from 0 to (RING_SIZE - 1)
inline static WORD freeSize()
{
    return (RING_SIZE - 1) - queueSize();
}

static union
{
    struct
    {
        unsigned disableRamWrite :1;
        unsigned isWaitingForCommand :1;
        unsigned sdiCopyEnabled :1;
        unsigned playData :1;
    };
    BYTE b;
} s_flags;

#define AUDIO_SINK_PORT (SINK_AUDIO_TYPE + BASE_SINK_PORT)
const rom Sink g_audioSink = { SINK_AUDIO_TYPE,
                                 0,
                                 AUDIO_SINK_PORT,
                                 &createAudioSink,
                                 &destroyAudioSink,
                                 &pollAudioSink };

// The TCP client socket of display listener
static TCP_SOCKET s_listenerSocket = INVALID_SOCKET;

static void createAudioSink()
{
	// Open the sever TCP channel
	s_listenerSocket = TCPOpen(0, TCP_OPEN_SERVER, AUDIO_SINK_PORT, TCP_PURPOSE_STREAM_TCP_SERVER);
	if (s_listenerSocket == INVALID_SOCKET)
	{
		fatal("MP3_SRV");
	}
        _ringStart = _ringEnd = 0;
        s_flags.b = 0; // no flags
        s_flags.isWaitingForCommand = 1;
}

static void destroyAudioSink()
{
	if (s_listenerSocket != INVALID_SOCKET)
	{
		TCPClose(s_listenerSocket);
	}
}

typedef enum
{
    AUDIO_INIT = 0,         // Reset the Audio HW
    AUDIO_SET_VOLUME = 1,   // Change volume
    AUDIO_TEST_SINE = 2,    // Start a sine test (reset to quit)
    AUDIO_STREAM_PACKET = 3,// Send data packet to SDI channel
    AUDIO_ENABLE_SDI = 4,   // Enable stream data to SDI MP3 channel
    AUDIO_STREAM_BEGIN = 5, // Start stream, suspend play waiting for a buffer almost full
    AUDIO_STREAM_END = 6,   // End stream, force play until buffer is empty

    AUDIO_TEST_1 = 100,      // Test, don't copy data in ext mem but get it from TCP
} AUDIO_COMMAND;  // 8-bit integer

typedef enum
{
    AUDIO_RES_OK = 0,
    AUDIO_RES_HW_FAIL = 1,      // MP3 hw fail (wrong model, internal test failed...)
    AUDIO_RES_SOCKET_ERR = 2,   // Missing data/buffer underrun on TCP/IP socket
} AUDIO_RESPONSE;  // 8-bit integer

typedef struct
{
    BYTE leftAttenuation;   // in db, 0 means full audio, 255 mute
    BYTE rightAttenuation;  // in db, 0 means full audio, 255 mute
} AUDIO_SET_VOLUME_DATA;

typedef struct
{
    UINT frequency;         // in Hz, using 48Khz sampling rate, will be rounded to next 375Hz step
} AUDIO_SINETEST_DATA;

typedef struct
{
    WORD packetSize;
} AUDIO_STREAM_DATA;

typedef struct
{
    AUDIO_RESPONSE res;
    UINT16 calls;
    INT32 freeSize;
} AUDIO_STREAM_RESPONSE;

static AUDIO_RESPONSE _initAudio()
{
    VS1011_MODEL r = vs1011_reset(FALSE);
    return (r != VS1011_MODEL_VS1011E) ? AUDIO_RES_HW_FAIL : AUDIO_RES_OK;
}

static AUDIO_RESPONSE _setVolume()
{
    AUDIO_SET_VOLUME_DATA msg;
    if (TCPGetArray(s_listenerSocket, (BYTE*)&msg, sizeof(AUDIO_SET_VOLUME_DATA)) != sizeof(AUDIO_SET_VOLUME_DATA))
    {
        return AUDIO_RES_SOCKET_ERR;
    }

    vs1011_volume(msg.leftAttenuation, msg.rightAttenuation);
    return AUDIO_RES_OK;
}

static AUDIO_RESPONSE _sineTest()
{
    AUDIO_SINETEST_DATA msg;
    if (TCPGetArray(s_listenerSocket, (BYTE*)&msg, sizeof(AUDIO_SINETEST_DATA)) != sizeof(AUDIO_SINETEST_DATA))
    {
        return AUDIO_RES_SOCKET_ERR;
    }

    vs1011_sineTest(msg.frequency);
    return AUDIO_RES_OK;
}

/*static*/ WORD _streamSize;
static void _processData(WORD len);
static int _dequeueCallCount;

static AUDIO_RESPONSE _startStream()
{
    // Read packet size
    _dequeueCallCount = 0;
    AUDIO_STREAM_DATA msg;
    if (TCPGetArray(s_listenerSocket, (BYTE*)&msg, sizeof(AUDIO_STREAM_DATA)) != sizeof(AUDIO_STREAM_DATA))
    {
        return AUDIO_RES_SOCKET_ERR;
    }
    _streamSize = msg.packetSize;

    // Now read _streamSize BYTES to RAM
    _processData(TCPIsGetReady(s_listenerSocket));
    return AUDIO_RES_OK;
}

void audio_pollMp3Player();

static void _processData(WORD len)
{
    _dequeueCallCount++;
    static BYTE buffer[1500];

    // If no room, leave the socket unread
    if (_streamSize > 0)
    {
        // Only dequeue a buffer of data
        WORD space = freeSize();
        if (len > space)
        {
            len = space;
        }
        if (len > _streamSize)
        {
            len = _streamSize;
        }
    }

    if (len > 0)
    {
        // Allocate bytes and transfer it to the external RAM ring buffer
        if (len > sizeof(buffer))
        {
            len = sizeof(buffer);
        }
        // Fetch data from eth RAM
        len = TCPGetArray(s_listenerSocket, buffer, len);
        // Poll audio
        audio_pollMp3Player();

        if (!s_flags.disableRamWrite)
        {
            // Copy data to Ext RAM
            // Calc free space (only valid when _ringEnd > 0)
            WORD space = (WORD)RING_SIZE - _ringEnd;
            if (space < len)
            {
                // No room for a single block copy.
                // Copy in two steps
                sram_write(buffer, MEM_OFFSET + _ringEnd, space);
                _ringEnd = 0;
                audio_pollMp3Player();
                WORD l = len - space;
                sram_write(buffer + space, MEM_OFFSET + 0, l);
                _ringEnd = l;
            }
            else
            {
                // Copy in one go
                sram_write(buffer, MEM_OFFSET + _ringEnd, len);
                //_ringEnd = (_ringEnd + l) % RING_SIZE;
                // It will cropped to 16 bit
                _ringEnd += len;
            }

            audio_pollMp3Player();
        }

        _streamSize -= len;
    }

    if (_streamSize == 0)
    {
        //TCPDiscard(s_listenerSocket);

        // Only do it if no other operation was done above
        AUDIO_STREAM_RESPONSE response;
        response.res = AUDIO_RES_OK;
        response.calls = _dequeueCallCount;
        response.freeSize = freeSize();

        // ACK
        s_flags.isWaitingForCommand = 1;
        
        if (TCPPutArray(s_listenerSocket, (BYTE*)&response, sizeof(AUDIO_STREAM_RESPONSE)) != sizeof(AUDIO_STREAM_RESPONSE))
        {
            fatal("MP3_ACK");
        }

        audio_pollMp3Player();

        TCPFlush(s_listenerSocket);

        audio_pollMp3Player();
    }
}

static void pollAudioSink()
{
	if (!TCPIsConnected(s_listenerSocket))
	{
            return;
	}

	WORD s = TCPIsGetReady(s_listenerSocket);
        if (s_flags.isWaitingForCommand)
        {
            if (s >= sizeof(AUDIO_COMMAND))
            {
                    AUDIO_COMMAND cmd;
                    TCPGetArray(s_listenerSocket, (BYTE*)&cmd, sizeof(AUDIO_COMMAND));
                    AUDIO_RESPONSE response = AUDIO_RES_OK;
                    switch (cmd)
                    {
                        case AUDIO_INIT:
                            response = _initAudio();
                            break;
                        case AUDIO_SET_VOLUME:
                            response = _setVolume();
                            break;
                        case AUDIO_TEST_SINE:
                            response = _sineTest();
                            break;
                        case AUDIO_ENABLE_SDI:
                            s_flags.sdiCopyEnabled = 1;
                            break;
                        case AUDIO_TEST_1:
                            s_flags.disableRamWrite = 1;
                            goto audioStream;
                        case AUDIO_STREAM_BEGIN:
                            s_flags.playData = 0;
                            break;
                        case AUDIO_STREAM_END:
                            s_flags.playData = 1;
                            break;
                        case AUDIO_STREAM_PACKET:
                            s_flags.disableRamWrite = 0;
audioStream:
                            response = _startStream();
                            if (response == AUDIO_RES_OK)
                            {
                                s_flags.isWaitingForCommand = 0;
                            }
                            break;
                    }

                    if (s_flags.isWaitingForCommand)
                    {
                        // ACK
                        if (TCPPutArray(s_listenerSocket, (BYTE*)&response, sizeof(AUDIO_RESPONSE)) != sizeof(AUDIO_RESPONSE))
                        {
                            fatal("MP3_SND");
                        }
                        TCPFlush(s_listenerSocket);
                    }
            }
        }
        else
        {
            _processData(s);
        }
}


void audio_pollMp3Player()
{
    static BYTE buffer[32];

    if (!s_flags.sdiCopyEnabled)
    {
        // Flush data in buffer
        _ringStart = _ringEnd;
        return;
    }

    WORD len = queueSize();
    if (!s_flags.playData)
    {
        // Buffer full, now you can play
        s_flags.playData = len > (RING_BUFFER_START_THRESHOLD);
    }

    // Can I play?
    if (s_flags.playData)
    {
        while (len >= sizeof(buffer) && vs1011_isWaitingData())
        {
            WORD avail = RING_SIZE - _ringStart;
            if (avail < sizeof(buffer))
            {
                sram_read(buffer, MEM_OFFSET + _ringStart, avail);
                sram_read(buffer + avail, MEM_OFFSET + 0, sizeof(buffer) - avail);
                _ringStart = sizeof(buffer) - avail;
            }
            else
            {
                // Pull 32 bytes of data
                sram_read(buffer, MEM_OFFSET + _ringStart, sizeof(buffer));
                _ringStart += sizeof(buffer);
            }

            len -= sizeof(buffer);

            // Flush data to MP3
            vs1011_streamData(buffer, sizeof(buffer));
        }
    }
}

#endif
