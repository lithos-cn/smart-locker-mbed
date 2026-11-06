#include "control.h"
#include <cassert>
#include <cstdio>
#include <limits>

using namespace locker;
static void close(float actual, float expected) { assert(std::fabs(actual-expected)<0.0001f); }

int main() {
    // Datasheet vectors and signed edge cases; these catch unsigned-char/sign-extension bugs.
    close(temperature_celsius(0x19,0x00),25.0f);
    close(temperature_celsius(0xe7,0x00),-25.0f);
    close(temperature_celsius(0xff,0xf0),-0.0625f);
    close(temperature_celsius(0x7f,0xf0),127.9375f);
    close(temperature_celsius(0x80,0x00),-128.0f);
    close(acceleration_g(0x80,0x00),0.512f);
    close(acceleration_g(0x06,0xff),-1.0f);
    close(acceleration_g(0xff,0xff),-0.004f);
    close(displacement_g({1,0,0},{1,0,0}),0.0f);
    close(displacement_g({0,0,1},{1,0,0}),std::sqrt(2.0f));

    DebouncedButton b;
    assert(!b.update(true,0));
    assert(!b.update(false,10));
    assert(!b.update(true,20));
    assert(!b.update(true,69));
    assert(b.update(true,70));
    assert(!b.update(true,500)); // Holding cannot repeat a digit.
    assert(!b.update(false,501));
    assert(!b.update(true,510)); // Release bounce cannot create a second press.
    assert(!b.update(false,520));
    assert(!b.update(false,570));
    assert(!b.pressed());
    assert(!b.update(true,600));
    assert(b.update(true,650));
    DebouncedButton left, right;
    assert(!left.update(true,0)); assert(!right.update(true,20));
    assert(left.update(true,50)); assert(!right.update(true,50));
    assert(right.update(true,70)); // Independent buttons never replace each other's pending state.
    DebouncedButton long_uptime;
    const std::uint64_t t = 0x100000000ULL;
    assert(!long_uptime.update(true,t)); assert(long_uptime.update(true,t+50));

    Password p;
    for (int run=0; run<2; ++run) {
        assert(p.push(1)==PasswordResult::Incomplete);
        assert(p.push(2)==PasswordResult::Incomplete);
        assert(p.push(1)==PasswordResult::Incomplete);
        assert(p.push(1)==PasswordResult::Accepted);
        assert(p.count()==0);
    }
    p.push(2); p.push(2); p.push(1);
    assert(p.push(1)==PasswordResult::Rejected);
    assert(p.push(3)==PasswordResult::Rejected && p.count()==0);
    p.push(1); p.reset(); assert(p.count()==0);

    Feedback feedback;
    feedback.show(true,100);
    auto out=outputs(true,true,25,0,feedback,500);
    assert(out.success && !out.buzzer);
    // A successful password must not suppress a concurrent environmental alarm.
    out=outputs(true,true,31,0,feedback,500);
    assert(out.success && out.temperature && out.buzzer);
    out=outputs(false,true,25,0,feedback,500);
    assert(out.temperature && out.buzzer);
    out=outputs(true,false,25,0,feedback,500);
    assert(out.motion && out.buzzer);
    out=outputs(true,true,std::numeric_limits<float>::quiet_NaN(),0,feedback,500);
    assert(out.temperature && out.buzzer);
    out=outputs(true,true,20,0.5f,feedback,1100);
    assert(!out.success && !out.temperature && !out.motion && !out.buzzer);
    feedback.show(false,2000);
    assert(outputs(true,true,25,0,feedback,2999).buzzer);
    assert(!outputs(true,true,25,0,feedback,3000).buzzer);
    std::puts("control tests passed");
}
