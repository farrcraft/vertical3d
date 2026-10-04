/**
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 **/

#include "SpriteClip.h"

#include <algorithm>
#include <stdexcept>
#include <vector>

namespace v3d::type::animation {

namespace {

/**
 * @return the summed length of the frames, refusing what a clip cannot show
 **/
float total(const std::vector<SpriteClip::Frame>& frames) {
    if (frames.empty()) {
        throw std::invalid_argument("a sprite clip needs at least one frame");
    }
    float sum = 0.0f;
    for (const SpriteClip::Frame& frame : frames) {
        if (!(frame.duration > 0.0f)) {
            throw std::invalid_argument("a sprite clip's frame must be shown for some time");
        }
        sum += frame.duration;
    }
    return sum;
}

};  // namespace

SpriteClip::SpriteClip(const std::vector<Frame>& frames, bool loops) :
    frames_(frames),
    clock_(total(frames), loops) {
    ends_.reserve(frames_.size());
    float end = 0.0f;
    for (const Frame& frame : frames_) {
        end += frame.duration;
        ends_.push_back(end);
    }
}

const Clock& SpriteClip::clock() const noexcept {
    return clock_;
}

const SpriteClip::Frame& SpriteClip::frame(float time) const noexcept {
    const float within = clock_.sample(time);
    const auto showing = std::upper_bound(ends_.begin(), ends_.end(), within);
    if (showing == ends_.end()) {
        return frames_.back();
    }
    return frames_[static_cast<std::size_t>(showing - ends_.begin())];
}

const std::vector<SpriteClip::Frame>& SpriteClip::frames() const noexcept {
    return frames_;
}

};  // namespace v3d::type::animation
