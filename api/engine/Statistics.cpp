/**
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 **/

#include "Statistics.h"

#include <algorithm>

namespace v3d::engine {

void Statistics::frame(std::uint64_t frame, unsigned int steps) noexcept {
    last_ = frame;
    steps_ = steps;
    // the slot being overwritten leaves the window, so the running total loses it rather
    // than the mean summing the array again every frame
    total_ -= recent_[next_];
    total_ += frame;
    recent_[next_] = frame;
    next_ = (next_ + 1) % window;
    frames_++;
}

std::uint64_t Statistics::last() const noexcept {
    return last_;
}

std::uint64_t Statistics::mean() const noexcept {
    if (frames_ == 0) {
        return 0;
    }
    return total_ / std::min<std::uint64_t>(frames_, window);
}

unsigned int Statistics::steps() const noexcept {
    return steps_;
}

std::uint64_t Statistics::frames() const noexcept {
    return frames_;
}

};  // namespace v3d::engine
