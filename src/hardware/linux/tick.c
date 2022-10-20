#include "../../pch.h"
#include "../tick.h"
#include <sys/time.h>

DWORD TickGet() {
    struct timeval tv;
    if (gettimeofday(&tv, NULL) != 0) {
        fatal("gettimeofday");
    }
    return ((tv.tv_sec * 1000000ul) + tv.tv_usec);
}

void timers_poll() {
    
}

void timers_init() {

}
