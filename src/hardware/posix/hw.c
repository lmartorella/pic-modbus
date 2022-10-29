#include "../../pch.h"

void CLRWDT()
{ }

void enableInterrupts() 
{ }

void rom_poll()
{ }

void sys_storeResetReason()
{ }

void fatal(const char* str) {
    fprintf(stderr, "FATAL: %s\n", str);
    exit(1);
}
