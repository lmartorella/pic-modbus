/*
 * To change this license header, choose License Headers in Project Properties.
 * To change this template file, choose Tools | Templates
 * and open the template in the editor.
 */

#include "../pch.h"

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
