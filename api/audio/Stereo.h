/*
 * Vertical3D
 * Copyright(c) 2023 Joshua Farr(josh@farrcraft.com)
 */

#pragma once

namespace v3d::audio {

/**
 * The gain of each side of a panned sound.
 **/
struct Stereo final {
    float left;
    float right;
};

/**
 * The gains a pan gives each side. The side the sound is panned towards stays at full gain. The
 * other falls in a straight line to silence at the end of the range, so a centred sound is
 * exactly an unpanned one. A pan past either end is held there, and one that is not finite is
 * the centre.
 **/
Stereo stereo(float pan) noexcept;

};  // namespace v3d::audio
