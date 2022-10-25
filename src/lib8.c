#include "pch.h"

/**
 * Code-memory optimized version of memcmp
 */
__bit memcmp8(void* p1, void* p2, uint8_t size)
{
    while (size-- > 0) {
        if (*((uint8_t*)p1) != *((uint8_t*)p2)) return 1;
        p1 = ((uint8_t*)p1) + 1;
        p2 = ((uint8_t*)p2) + 1;
    }
    return 0;
}
