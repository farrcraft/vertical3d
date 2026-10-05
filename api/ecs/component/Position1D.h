/**
 * Vertical3D
 * Copyright(c) 2023 Joshua Farr(josh@farrcraft.com)
 **/

#pragma once

namespace v3d::ecs::component {

/**
 * Where a thing is along a line. An aggregate, so a snapshot of the previous step is a plain
 * copy, and written directly, since simulation sets it every step.
 **/
struct Position1D final {
    float value = 0.0f;
};

/**
 * The position alpha of the way from one to the other, which ecs::interpolated uses to draw
 * it between two simulation steps.
 **/
Position1D interpolate(const Position1D& from, const Position1D& to, float alpha);

};  // namespace v3d::ecs::component
