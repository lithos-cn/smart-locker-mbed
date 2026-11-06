#include "adxl345.h"
#include "tmp102.h"
#include <cassert>
#include <cstdio>

namespace fake {
std::array<std::uint8_t,64> registers{};
std::array<std::uint16_t,4> i2c_registers{};
bool selected=false,command_pending=true,read_mode=false,increment=false,fail_write=false,fail_read=false;
unsigned cursor=0,pointer=0;
}

int main() {
    ADXL345 accelerometer(A6,A5,A4,D12);
    assert(!accelerometer.begin()); // Missing device must fail startup.
    fake::registers[0]=0xe5;
    assert(accelerometer.begin());
    assert(fake::registers[0x31]==0x08 && fake::registers[0x2c]==0x0a && fake::registers[0x2d]==0x08);
    fake::registers[0x32]=0x80; fake::registers[0x33]=0x00;
    fake::registers[0x34]=0x06; fake::registers[0x35]=0xff;
    fake::registers[0x36]=0xfa; fake::registers[0x37]=0x00;
    locker::Acceleration sample{};
    assert(accelerometer.read(sample));
    assert(std::fabs(sample.x-0.512f)<0.0001f && std::fabs(sample.y+1.0f)<0.0001f && std::fabs(sample.z-1.0f)<0.0001f);
    assert(!fake::selected);
    fake::registers[0]=0;
    assert(!accelerometer.read(sample)); // Device disconnection is not a valid zero reading.

    TMP102 thermometer(PA_10,PA_9);
    assert(thermometer.begin());
    assert(fake::i2c_registers[1]==0x60a0); // Continuous, normal format, 4 Hz.
    fake::i2c_registers[0]=0xe700;
    float temperature=0;
    assert(thermometer.read(temperature) && temperature==-25.0f);
    fake::i2c_registers[0]=0x1980;
    assert(thermometer.read(temperature) && temperature==25.5f);
    fake::fail_read=true;
    assert(!thermometer.read(temperature) && temperature==25.5f);
    assert(!thermometer.begin());
    fake::fail_read=false; fake::fail_write=true;
    assert(!thermometer.begin());
    assert(!thermometer.read(temperature));
    std::puts("driver tests passed");
}
