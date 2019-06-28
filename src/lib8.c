#include "pch.h"

/**
 * Code-memory optimized version of memcmp
 */
bit memcmp8(void* p1, void* p2, BYTE size)
{
    while (size-- > 0) {
        if (*((BYTE*)p1) != *((BYTE*)p2)) return 1;
        p1 = ((BYTE*)p1) + 1;
        p2 = ((BYTE*)p2) + 1;
    }
    return 0;
}
