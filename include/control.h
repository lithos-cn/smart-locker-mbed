#pragma once

#include <cmath>
#include <cstddef>
#include <cstdint>

namespace locker {

struct Acceleration { float x, y, z; };

inline float temperature_celsius(std::uint8_t high, std::uint8_t low) {
    int raw = (static_cast<unsigned>(high) << 4) | (low >> 4);
    if (raw & 0x800) raw -= 4096;
    return raw * 0.0625f;
}

inline float acceleration_g(std::uint8_t low, std::uint8_t high) {
    const unsigned raw = (static_cast<unsigned>(high) << 8) | low;
    const int signed_raw = raw >= 0x8000 ? static_cast<int>(raw) - 65536 : raw;
    return signed_raw * 0.004f;
}

inline float displacement_g(const Acceleration &value, const Acceleration &baseline) {
    const float x = value.x - baseline.x;
    const float y = value.y - baseline.y;
    const float z = value.z - baseline.z;
    return std::sqrt(x*x + y*y + z*z);
}

// One independent stable-state debouncer per active-low button.
class DebouncedButton {
public:
    bool update(bool pressed, std::uint64_t now_ms) {
        if (pressed != raw_pressed_) {
            raw_pressed_ = pressed;
            changed_at_ = now_ms;
        }
        if (stable_pressed_ != raw_pressed_ && now_ms - changed_at_ >= 50) {
            stable_pressed_ = raw_pressed_;
            return stable_pressed_;
        }
        return false;
    }
    bool pressed() const { return stable_pressed_; }
private:
    bool raw_pressed_ = false;
    bool stable_pressed_ = false;
    std::uint64_t changed_at_ = 0;
};

enum class PasswordResult { Incomplete, Accepted, Rejected };

class Password {
public:
    PasswordResult push(int digit) {
        if (digit != 1 && digit != 2) { reset(); return PasswordResult::Rejected; }
        matched_ = matched_ && digit == expected_[position_];
        if (++position_ != 4) return PasswordResult::Incomplete;
        const bool accepted = matched_;
        reset();
        return accepted ? PasswordResult::Accepted : PasswordResult::Rejected;
    }
    void reset() { position_ = 0; matched_ = true; }
    std::size_t count() const { return position_; }
private:
    const int expected_[4] = {1, 2, 1, 1};
    std::size_t position_ = 0;
    bool matched_ = true;
};

class Feedback {
public:
    void show(bool accepted, std::uint64_t now_ms) {
        accepted_ = accepted; active_ = true; until_ = now_ms + 1000;
    }
    bool active(std::uint64_t now_ms) const { return active_ && now_ms < until_; }
    bool success(std::uint64_t now_ms) const { return active(now_ms) && accepted_; }
    bool failure(std::uint64_t now_ms) const { return active(now_ms) && !accepted_; }
private:
    bool accepted_ = false, active_ = false;
    std::uint64_t until_ = 0;
};

struct Outputs { bool temperature, motion, success, denied, buzzer; };

inline Outputs outputs(bool temperature_ok, bool acceleration_ok,
                       float temperature, float motion,
                       const Feedback &feedback, std::uint64_t now_ms) {
    const bool hot_or_cold = !temperature_ok || !std::isfinite(temperature)
                            || temperature < 20.0f || temperature > 30.0f;
    const bool moved = !acceleration_ok || !std::isfinite(motion) || motion > 0.5f;
    return {hot_or_cold, moved, feedback.success(now_ms), feedback.failure(now_ms),
            hot_or_cold || moved || feedback.failure(now_ms)};
}
} // namespace locker
