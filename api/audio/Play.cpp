/**
 * Vertical3D
 * Copyright(c) 2023 Joshua Farr(josh@farrcraft.com)
 **/

#include "Play.h"

#include <algorithm>
#include <cmath>

namespace v3d::audio {

/**
 **/
Play::Play() noexcept :
loops(0),
fadeInMs(0),
gain(1.0f),
pan(0.0f) {
}

/**
 **/
Stereo stereo(float pan) noexcept {
    const float held = std::isfinite(pan) ? std::clamp(pan, -1.0f, 1.0f) : 0.0f;
    return Stereo{std::min(1.0f, 1.0f - held), std::min(1.0f, 1.0f + held)};
}

};  // namespace v3d::audio
