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
gain(1.0F),
pan(0.0F) {
}

/**
 **/
Stereo stereo(float pan) noexcept {
    const float held = std::isfinite(pan) ? std::clamp(pan, -1.0F, 1.0F) : 0.0F;
    return Stereo{std::min(1.0F, 1.0F - held), std::min(1.0F, 1.0F + held)};
}

};  // namespace v3d::audio
