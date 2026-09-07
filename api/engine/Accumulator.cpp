/**
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 **/

#include "Accumulator.h"

namespace v3d::engine {

unsigned int Accumulator::accumulate(std::uint64_t elapsed) noexcept {
    remainder_ += (elapsed > clamp) ? clamp : elapsed;
    owed_ = static_cast<unsigned int>(remainder_ / step);
    steps_ = owed_;
    return owed_;
}

bool Accumulator::drain() noexcept {
    if (owed_ == 0) {
        return false;
    }
    --owed_;
    remainder_ -= step;
    simulated_ += step;
    return true;
}

float Accumulator::alpha() const noexcept {
    // whole steps are only outstanding mid-drain, and a caller asking then is asking about a
    // frame it has not finished simulating - report the fraction either way
    return static_cast<float>(remainder_ % step) / static_cast<float>(step);
}

unsigned int Accumulator::steps() const noexcept {
    return steps_;
}

std::uint64_t Accumulator::simulated() const noexcept {
    return simulated_;
}

};  // namespace v3d::engine
