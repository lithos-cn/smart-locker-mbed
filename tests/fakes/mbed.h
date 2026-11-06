#pragma once
// Host test double, not a replacement for Mbed or hardware validation.
#include <array>
#include <cassert>
#include <chrono>
#include <cstdint>

enum PinName { A0=0,A4=4,A5=5,A6=6,D2=12,D3=13,D6=16,D9=19,D10=20,D11=21,D12=22,PA_10=30,PA_9=31 };
enum PinMode { PullUp };
namespace fake {
extern std::array<std::uint8_t,64> registers;
extern std::array<std::uint16_t,4> i2c_registers;
extern bool selected, command_pending, read_mode, increment, fail_write, fail_read;
extern unsigned cursor, pointer;
}
class DigitalOut {
public:
    DigitalOut(PinName pin,int value=0):pin_(pin) { *this=value; }
    DigitalOut &operator=(int value) {
        if(pin_==D12) { fake::selected=value==0; fake::command_pending=true; }
        return *this;
    }
private: PinName pin_;
};
class SPI {
public:
    SPI(PinName,PinName,PinName) {}
    void format(int bits,int mode) { assert(bits==8 && mode==3); }
    void frequency(int hz) { assert(hz==2000000); }
    int write(int data) {
        assert(fake::selected);
        if(fake::command_pending) {
            fake::command_pending=false; fake::read_mode=data&0x80;
            fake::increment=data&0x40; fake::cursor=data&0x3f; return 0;
        }
        int result=0;
        if(fake::read_mode) result=fake::registers.at(fake::cursor);
        else fake::registers.at(fake::cursor)=static_cast<std::uint8_t>(data);
        if(fake::increment) ++fake::cursor;
        return result;
    }
};
class I2C {
public:
    I2C(PinName,PinName) {}
    void frequency(int hz) { assert(hz==100000); }
    int write(int address,const char *data,int size,bool repeated=false) {
        assert(address==0x90);
        if(fake::fail_write) return 1;
        fake::pointer=static_cast<unsigned char>(data[0]);
        if(size==1) assert(repeated); // Pointer write must precede a repeated-start read.
        else {
            assert(size==3 && !repeated);
            fake::i2c_registers.at(fake::pointer)=
                (static_cast<unsigned char>(data[1])<<8)|static_cast<unsigned char>(data[2]);
        }
        return 0;
    }
    int read(int address,char *data,int size,bool repeated=false) {
        assert(address==0x90 && size==2 && !repeated);
        if(fake::fail_read) return 1;
        const auto value=fake::i2c_registers.at(fake::pointer);
        data[0]=static_cast<char>(value>>8); data[1]=static_cast<char>(value&0xff);
        return 0;
    }
};
class InterruptIn {
public:
    InterruptIn(PinName,PinMode) {}
    void fall(void (*)()) {}
    int read() { return 1; }
};
class PwmOut {
public:
    explicit PwmOut(PinName) {}
    void period_us(int) {}
    void write(float) {}
};
namespace ThisThread {
template<class Rep,class Period> void sleep_for(std::chrono::duration<Rep,Period>) {}
}
namespace rtos {
namespace Kernel {
struct Clock {
    static std::chrono::time_point<std::chrono::steady_clock,std::chrono::milliseconds> now() {
        return {};
    }
};
}
class EventFlags {
public:
    std::uint32_t set(std::uint32_t) { return 0; }
    std::uint32_t wait_any(std::uint32_t,std::uint32_t) { return 0; }
};
}
