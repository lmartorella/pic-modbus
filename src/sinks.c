#include "net.h"

static const TWOCC ResetCode = { "RS" };
static const TWOCC ExceptionText = { "EX" };
static const TWOCC EndOfMetadataText = { "EN" };
static const TWOCC BusMasterStats = { "BM" };
static __bit s_isPrimary;

enum SYSSINK_CMD {
    SYSSINK_CMD_RESET = 1,
    SYSSINK_CMD_CLRRST = 2,
};

#define SYS_READ_IMPL(IMPL) \
    if (prot_ ## IMPL ## _control_readAvail() < 1) { \
        /* Wait cmd */ \
        return 1; \
    } \
    uint8_t cmd; \
    prot_  ## IMPL ## _control_read(&cmd, 1); \
    switch (cmd) { \
        case SYSSINK_CMD_RESET: \
            /* Reset device (not as MClr zeroconf) */ \
            fatal("RST"); \
            break; \
        case SYSSINK_CMD_CLRRST: \
            /* Reset reset reason */ \
            g_resetReason = RESET_NONE; \
            break; \
    } \
    /* No more data */ \
    return false; \

__bit sys_read_prim() {
    SYS_READ_IMPL(prim);
}
__bit sys_read_sec() {
    SYS_READ_IMPL(sec);
}

#define SYS_WRITE_IMPL_HEADER(IMPL) \
    uint16_t l = g_resetReason; \
    /* Write reset reason */ \
    prot_ ## IMPL ## _control_write(&ResetCode, sizeof(TWOCC)); \
    prot_ ## IMPL ## _control_writeW(l); \
\
    if (g_resetReason == RESET_EXC) { \
        prot_ ## IMPL ## _control_write(&ExceptionText, sizeof(TWOCC)); \
\
        const char *exc = (const char *)g_lastException; \
        l = strlen(exc); \
        if (l > 12) { \
            l = 12; \
        } \
        prot_ ## IMPL ## _control_writeW(l); \
        prot_ ## IMPL ## _control_write(exc, l); \
    } \

__bit sys_write_prim() {
    SYS_WRITE_IMPL_HEADER(prim);
    prot_prim_control_write(&BusMasterStats, sizeof(TWOCC));
    prot_prim_control_write(&bus_prim_busStats, sizeof(BUS_PRIMARY_STATS));
    memset(&bus_prim_busStats, 0, sizeof(BUS_PRIMARY_STATS));

    prot_prim_control_write(&EndOfMetadataText, sizeof(TWOCC));
    // Finish
    return false;
}

__bit sys_write_sec() {
    SYS_WRITE_IMPL_HEADER(sec);
    prot_sec_control_write(&EndOfMetadataText, sizeof(TWOCC)); \
    // Finish
    return false;
}
