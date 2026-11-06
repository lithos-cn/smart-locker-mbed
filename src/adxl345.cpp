#include "adxl345.h"

ADXL345::ADXL345(PinName mosi, PinName miso, PinName clock, PinName chip_select)
    : spi_(mosi, miso, clock), cs_(chip_select, 1) {}

void ADXL345::write_register(std::uint8_t reg, std::uint8_t value) {
    cs_ = 0;
    spi_.write(reg & 0x3f);
    spi_.write(value);
    cs_ = 1;
}

std::uint8_t ADXL345::read_register(std::uint8_t reg) {
    cs_ = 0;
    spi_.write(0x80 | reg);
    const auto value = static_cast<std::uint8_t>(spi_.write(0));
    cs_ = 1;
    return value;
}

bool ADXL345::begin() {
    spi_.format(8, 3);
    spi_.frequency(2000000);
    if (read_register(0x00) != 0xe5) return false;
    write_register(0x2d, 0x00); // Standby while configuring.
    write_register(0x31, 0x08); // Four-wire SPI, full resolution, +/-2 g.
    write_register(0x2c, 0x0a); // 100 Hz output data rate.
    write_register(0x2d, 0x08); // Measurement mode.
    return (read_register(0x31) & 0x0f) == 0x08
        && (read_register(0x2d) & 0x08) != 0;
}

bool ADXL345::read(locker::Acceleration &value) {
    // SPI has no acknowledgement; checking DEVID catches common disconnection faults.
    if (read_register(0x00) != 0xe5) return false;
    std::uint8_t bytes[6];
    cs_ = 0;
    spi_.write(0xc0 | 0x32); // Read six consecutive bytes, starting at DATAX0.
    for (auto &byte : bytes) byte = static_cast<std::uint8_t>(spi_.write(0));
    cs_ = 1;
    value = {locker::acceleration_g(bytes[0], bytes[1]),
             locker::acceleration_g(bytes[2], bytes[3]),
             locker::acceleration_g(bytes[4], bytes[5])};
    return true;
}
