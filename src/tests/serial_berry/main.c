#define _GNU_SOURCE

#include <stdlib.h>
#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include <unistd.h>
#include <math.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <errno.h> 

#include "../../pch.h"
#include "../../rs485.h"

void CLRWDT()
{ }

void fatal(const char* err) {
    fprintf(stderr, "%s: %d\n", err, errno);
    exit(1);
}

static void fatalw(const char* str, uint32_t data) {
    printf("%s: 0x%08x\n", str, data);
}

int main() {
    rs485_init();

    // Writes packet
    //char* data = "luxsoftware17";
    char* data = "\x41";
    char* parStr = strdup(data);
    short size = strlen(data);
    
    char buf[] = { (char)0x55, (char)0xaa };
    rs485_write(0, buf, 2);
    rs485_write(0, (char*)&size, 2);
    rs485_write(0, data, size);
    
    printf("Packet written, %hd byte\n", size);
    
    // Now receive data
    int i = -1;
    while (i < size) {
        rs485_interrupt();
        rs485_poll();
        while (rs485_readAvail() > 0 && i < size) {
            char buf;
            rs485_read(&buf, 1);
            int is9 = rs485_lastRc9;
            printf("%c %d\n", buf, is9);
            i++;
        }
    }

    return 0;
}
