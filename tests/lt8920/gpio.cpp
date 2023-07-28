#include <fcntl.h>
#include <unistd.h>

#include <fstream>
#include <iostream>
#include <filesystem>
#include <stdexcept>

#include "./gpio.h"

using namespace std::chrono;
using namespace std::string_literals;

GpioDigitalPin::GpioDigitalPin(int gpio, const std::string& direction)
    :gpio(std::to_string(gpio)), exported(false)
{
    // Export the pin (if not exported already)
    {
        std::ofstream exportF("/sys/class/gpio/export");
        if (!exportF) {
            throw std::runtime_error("Cannot open export stream");
        }
        exportF << gpio;
    }
    exported = true;

    if (!std::filesystem::exists("/sys/class/gpio/gpio"s + this->gpio)) {
        throw std::runtime_error("GPIO "s + this->gpio + " not exported correctly"s);
    }

    // WTF
    usleep(100 * 1000);

    // Configure direction
    {
        std::ofstream directionF("/sys/class/gpio/gpio"s + this->gpio + "/direction"s);
        if (!directionF) {
            throw std::runtime_error("Cannot set gpio "s + this->gpio + " direction");
        }
        directionF << direction;
    }
}

GpioDigitalPin::GpioDigitalPin(GpioDigitalPin&& move)
    :exported(move.exported), gpio(move.gpio) 
{
    move.exported = false;
}

GpioDigitalPin::~GpioDigitalPin() {
    if (exported) {
        std::ofstream exportF;
        exportF.open("/sys/class/gpio/unexport");
        if (exportF) {
            exportF << gpio;
        }
    }
}

OutputPin::OutputPin(int gpio)
    :GpioDigitalPin(gpio, "out") 
{ }

OutputPin& OutputPin::operator= (bool value) {
    std::ofstream valueF("/sys/class/gpio/gpio"s + gpio + "/value"s);
    if (!valueF) {
        throw std::runtime_error("Cannot set the pin direction");
    }
    valueF << value ? "1" : "0";
    return *this;
}

InputPin::InputPin(int gpio)
    :GpioDigitalPin(gpio, "in"), fd(0) 
{ 
    // Now open the value stream for select
    fd = open(("/sys/class/gpio/gpio"s + this->gpio + "/value"s).c_str(), O_RDONLY);
    if (fd < 0) {
        throw std::runtime_error("Cannot open gpio "s + this->gpio + " value stream"s);
    }
    value();
}

InputPin::InputPin(InputPin&& move) 
    :GpioDigitalPin(std::move(move)), fd(move.fd)
{
    move.fd = 0;
}

InputPin::~InputPin() {
    if (fd > 0) {
        close(fd);
    }
}

bool InputPin::value() {
    // Read value to reset exceptional condition
    if (lseek(fd, 0, SEEK_SET) != 0) {
        throw std::runtime_error("Cannot seek gpio fs");
    }
    char buffer[10];
    int n = read(fd, buffer, sizeof(buffer));
    buffer[n] = 0;
    return buffer[0] != '0';
}
