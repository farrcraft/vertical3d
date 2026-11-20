/*
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 */

#pragma once

#include <cstdint>

namespace v3d::ui {

/**
 * The time a ui is drawn at: seconds on the ui's own clock, and how many frames have been drawn.
 *
 * The screen advances it by the real time between frames, not by the simulation's steps, so a
 * menu still eases open while the game behind it is paused. A test sets it directly.
 **/
struct Clock final {
    float now = 0.0F;
    std::uint64_t frame = 0;
};

};  // namespace v3d::ui
