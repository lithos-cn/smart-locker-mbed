#include "tmp102.h"
#include "control.h"

TMP102::TMP102(PinName sda, PinName scl, int address_7bit)
    : i2c_(sda, scl), address_(address_7bit << 1) {}

bool TMP102::read_register(char reg, char (&bytes)[2]) {
    if (i2c_.write(address_, &reg, 1, true) != 0) return false;
    return i2c_.read(address_, bytes, 2) == 0;
}

bool TMP102::begin() {
    i2c_.frequency(100000);
    // SD=0: continuous conversion; EM=0: signed 12-bit; CR=10: 4 Hz.
    // ALERT is unused; software applies the 20..30 C interval.
    const char configuration[] = {0x01, 0x60, static_cast<char>(0xa0)};
    if (i2c_.write(address_, configuration, 3) != 0) return false;
    char actual[2];
    if (!read_register(0x01, actual)) return false;
    const unsigned high = static_cast<unsigned char>(actual[0]);
    const unsigned low = static_cast<unsigned char>(actual[1]);
    return (high & 0x01) == 0 && (low & 0x10) == 0 && (low & 0xc0) == 0x80;
}

bool TMP102::read(float &temperature) {
    char bytes[2];
    if (!read_register(0x00, bytes)) return false;
    temperature = locker::temperature_celsius(static_cast<unsigned char>(bytes[0]),
                                              static_cast<unsigned char>(bytes[1]));
    return true;
}
