/**
 * Vertical3D
 * Copyright(c) 2023 Joshua Farr(josh@farrcraft.com)
 **/

#pragma once

#include <glm/vec2.hpp>

namespace v3d::ecs::component {

/**
 * Where a thing is on a plane. An aggregate, so a snapshot of the previous step is a plain
 * copy, and written directly, since simulation sets it every step.
 **/
struct Position2D final {
    glm::vec2 value{0.0f};
};

/**
 * The position alpha of the way from one to the other, which ecs::interpolated uses to draw
 * it between two simulation steps.
 **/
Position2D interpolate(const Position2D& from, const Position2D& to, float alpha);

};  // namespace v3d::ecs::component
