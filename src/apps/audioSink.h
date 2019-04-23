#ifndef AUDIO_SINK_H
#define	AUDIO_SINK_H

#ifdef HAS_BUS
#ifdef HAS_VS1011

#include "../protocol.h"
// The public sink descriptor
extern const Sink g_audioSink;

// Poll 
void audio_pollMp3Player();

#endif
#endif

#endif	/* AUDIO_SINK_H */

