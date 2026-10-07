/**
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 **/

#include "Clock.h"

#include <algorithm>
#include <cmath>
#include <cstdint>
#include <limits>

namespace v3d::type::animation {

Clock::Clock(float duration, bool loops) noexcept :
    duration_(duration),
    loops_(loops) {
}

float Clock::duration() const noexcept {
    return duration_;
}

bool Clock::loops() const noexcept {
    return loops_;
}

float Clock::advance(float time, float step) const noexcept {
    if (duration_ <= 0.0f) {
        return 0.0f;
    }
    if (loops_) {
        return time + step;
    }
    return std::clamp(time + step, 0.0f, duration_);
}

float Clock::sample(float time) const noexcept {
    if (duration_ <= 0.0f) {
        return 0.0f;
    }
    if (!loops_) {
        return std::clamp(time, 0.0f, duration_);
    }
    const float wrapped = std::fmod(time, duration_);
    return wrapped < 0.0f ? wrapped + duration_ : wrapped;
}

uint32_t Clock::crossed(float from, float to, float marker) const noexcept {
    if (!std::isfinite(from) || !std::isfinite(to) || std::isnan(marker)) {
        return 0;
    }
    if (duration_ <= 0.0f || to <= from || marker < 0.0f || marker > duration_) {
        return 0;
    }
    if (!loops_) {
        return from < marker && marker <= to ? 1 : 0;
    }
    // every k for which marker + k * duration lies in (from, to]
    const double length = duration_;
    const double first = std::floor((static_cast<double>(from) - marker) / length) + 1.0;
    const double last = std::floor((static_cast<double>(to) - marker) / length);
    if (last < first) {
        return 0;
    }
    const double count = last - first + 1.0;
    constexpr double most = std::numeric_limits<uint32_t>::max();
    return count >= most ? std::numeric_limits<uint32_t>::max() : static_cast<uint32_t>(count);
}

bool Clock::finished(float time) const noexcept {
    return !loops_ && time >= duration_;
}

};  // namespace v3d::type::animation
