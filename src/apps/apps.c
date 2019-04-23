#include "../pch.h"
#include "audioSink.h"
#include "bmp180app.h"
#include "dcf77.h"
#include "spiram.h"

void apps_init() 
{
#ifdef HAS_VS1011
    vs1011_init();
#endif

#ifdef HAS_DCF77
    dcf77_init();
#endif

#ifdef HAS_BMP180_APP
    bmp180_app_init();
#endif      
}

void apps_poll() 
{
#if HAS_VS1011
    audio_pollMp3Player();
#endif

#ifdef HAS_DCF77
    dcf77_poll();
#endif

#ifdef HAS_BMP180_APP
    bmp180_app_poll();
#endif
}
