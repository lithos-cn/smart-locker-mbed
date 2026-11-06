#pragma once
#include "mbed.h"

class TMP102 {
public:
    TMP102(PinName sda, PinName scl, int address_7bit = 0x48);
    bool begin();
    bool read(float &temperature);
private:
    bool read_register(char reg, char (&bytes)[2]);
    I2C i2c_;
    int address_;
};
