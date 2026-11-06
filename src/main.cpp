#include "mbed.h"
#include "adxl345.h"
#include "tmp102.h"
#include "control.h"
#include <chrono>
#include <cstdio>

using namespace std::chrono_literals;

namespace {
ADXL345 accelerometer(A6, A5, A4, D12);
TMP102 thermometer(PA_10, PA_9);
DigitalOut temperature_led(A0, 0), motion_led(D10, 0);
DigitalOut success_led(D6, 0), denied_led(D11, 0);
PwmOut buzzer(D9);
InterruptIn button_one(D2, PullUp), button_two(D3, PullUp);
rtos::EventFlags button_events;
constexpr std::uint32_t button_flags = 0x03;

// ISRs only wake the main thread. No timers, delays, bus transactions or input arrays here.
void on_button_one() { button_events.set(0x01); }
void on_button_two() { button_events.set(0x02); }

bool calibrate(locker::Acceleration &baseline) {
    locker::Acceleration total{0, 0, 0};
    for (unsigned i = 0; i < 32; ++i) {
        locker::Acceleration sample{};
        if (!accelerometer.read(sample)) return false;
        total.x += sample.x; total.y += sample.y; total.z += sample.z;
        ThisThread::sleep_for(20ms);
    }
    baseline = {total.x / 32, total.y / 32, total.z / 32};
    const float gravity = std::sqrt(baseline.x*baseline.x + baseline.y*baseline.y
                                   + baseline.z*baseline.z);
    return gravity >= 0.7f && gravity <= 1.3f;
}
}

int main() {
    buzzer.period_us(1000); // 1 kHz tone for a passive piezo.
    buzzer.write(0.0f);
    std::printf("Smart locker prototype: keep the board still during startup calibration.\n");
    const bool acceleration_configured = accelerometer.begin();
    const bool temperature_configured = thermometer.begin();
    ThisThread::sleep_for(300ms); // Allow sensor startup and the first temperature conversion.
    locker::Acceleration baseline{};
    const bool baseline_valid = acceleration_configured && calibrate(baseline);
    if (!baseline_valid || !temperature_configured)
        std::printf("Startup sensor fault; check wiring and reset the board.\n");

    button_one.fall(&on_button_one);
    button_two.fall(&on_button_two);
    locker::DebouncedButton first_button, second_button;
    locker::Password password;
    locker::Feedback feedback;
    bool acceleration_ok = false, temperature_ok = false;
    float motion = 0.0f, temperature = 0.0f;
    std::uint64_t next_acceleration = 0, next_temperature = 0, next_log = 0;

    while (true) {
        const auto now = rtos::Kernel::get_ms_count();
        const bool one = first_button.update(button_one.read() == 0, now);
        const bool two = second_button.update(button_two.read() == 0, now);
        if (one || two) {
            if (first_button.pressed() && second_button.pressed()) {
                // Reject ambiguous overlapping inputs instead of guessing their order.
                password.reset();
                feedback.show(false, now);
            } else if (!feedback.active(now)) {
                const auto result = password.push(one ? 1 : 2);
                if (result != locker::PasswordResult::Incomplete)
                    feedback.show(result == locker::PasswordResult::Accepted, now);
            }
        }

        if (now >= next_acceleration) {
            locker::Acceleration sample{};
            acceleration_ok = baseline_valid && accelerometer.read(sample);
            if (acceleration_ok) motion = locker::displacement_g(sample, baseline);
            next_acceleration = now + 100;
        }
        if (now >= next_temperature) {
            temperature_ok = temperature_configured && thermometer.read(temperature);
            next_temperature = now + 250;
        }

        // Timed feedback never blocks sensing. Sensor faults also activate the relevant alarm.
        const auto state = locker::outputs(temperature_ok, acceleration_ok,
                                           temperature, motion, feedback, now);
        temperature_led = state.temperature;
        motion_led = state.motion;
        success_led = state.success;
        denied_led = state.denied;
        buzzer.write(state.buzzer ? 0.5f : 0.0f);
        if (now >= next_log) {
            std::printf("temp_ok=%d temp_mC=%ld accel_ok=%d motion_mg=%ld digits=%u\n",
                        temperature_ok, static_cast<long>(temperature * 1000),
                        acceleration_ok, static_cast<long>(motion * 1000),
                        static_cast<unsigned>(password.count()));
            next_log = now + 1000;
        }
        // Interrupts wake this wait early; the timeout also tracks releases and stable levels.
        button_events.wait_any(button_flags, 5);
    }
}
