#include "net/net.h"

EXC_STRING_T g_lastException = EXC_STRING_NULL;
RESET_REASON g_resetReason;

void io_init() {
}

void io_println(const char* str) {
    printf("%s\r\n", str);
    fflush(stdout);
}

void io_printlnStatus(const char* str) {
    printf("%s\r\n", str);
    fflush(stdout);
}

void io_printChDbg(char ch) {
    printf("%c", ch);
    fflush(stdout);
}

void flog(const char* format, ...) {
    va_list args;
    va_start(args, format);

    vprintf(format, args);
    printf("\n");
    fflush(stdout);
}

void fatal(const char* str) {
    fprintf(stderr, "FATAL: %s\n", str);
    exit(1);
}
