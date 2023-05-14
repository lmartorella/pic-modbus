#include <stdexcept>

#include "configuration.h"
#include "pic-modbus/sys.h"

extern "C" {
    void RESET() {
        std::string reason;
        switch (sys_resetReason) {
            case EXC_CODE_RS485_READ_UNDERRUN:
                reason = "EXC_CODE_RS485_READ_UNDERRUN";
                break;
            case EXC_CODE_RS485_READ_OVERRUN:
                reason = "EXC_CODE_RS485_READ_OVERRUN";
                break;
            default:
                reason = std::to_string(sys_resetReason);
                break;
        }
        throw std::runtime_error("Fatal " + reason);
    }
}