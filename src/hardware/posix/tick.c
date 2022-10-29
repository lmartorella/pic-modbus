#include "../../pch.h"
#include "../../timers.h"
#include <sys/time.h>

uint32_t timers_get() {
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
