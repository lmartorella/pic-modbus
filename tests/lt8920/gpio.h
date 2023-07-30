#pragma once

#include <chrono>
#include <string>

/**
 * A I/O digital pin, via Linux sysfs
 */
class GpioDigitalPin {
    bool exported;
protected:
    const std::string gpio;
    GpioDigitalPin(int gpio, const std::string& direction);
    GpioDigitalPin(GpioDigitalPin&& move);
public:
    ~GpioDigitalPin();
};

/**
 * Selectable input
 * Deprecated. Use gpio-keys and event streams
 */
class InputPin : protected GpioDigitalPin { 
    int fd;
public:
    InputPin(int gpio);
    InputPin(InputPin&& copy);
    ~InputPin();

    // For select/poll usage. Generates a exception event in case of state change.
    int getFd() const {
        return fd;
    }

    /**
     * Call this after select/poll to reset state of file descriptor
     * Check if the event is due to contact bounce
     */
    bool value();
};

class OutputPin : protected GpioDigitalPin { 
public:
    OutputPin(int gpio, bool initialValue);
    OutputPin& operator= (bool value);
};
