/*
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 */

#pragma once

namespace v3d::ui::component {

/**
 * A scrollbar's track and its thumb's length, and the offset into the span it shows.
 **/
struct Thumb {
    float track = 0.0F;
    float length = 0.0F;
    float offset = 0.0F;
    float span = 0.0F;
};

};  // namespace v3d::ui::component
