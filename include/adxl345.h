#pragma once
#include "mbed.h"
#include "control.h"

class ADXL345 {
public:
    ADXL345(PinName mosi, PinName miso, PinName clock, PinName chip_select);
    bool begin();
    bool read(locker::Acceleration &value);
private:
    void write_register(std::uint8_t reg, std::uint8_t value);
    std::uint8_t read_register(std::uint8_t reg);
    SPI spi_;
    DigitalOut cs_;
};
