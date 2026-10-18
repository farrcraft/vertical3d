/**
 * Vertical3D
 * Copyright(c) 2023 Joshua Farr(josh@farrcraft.com)
 **/

#pragma once

#include <glm/vec2.hpp>

namespace v3d::ecs::component {

/**
 * An entity's position on a plane. A plain aggregate: snapshot() copies it, and simulation
 * writes value directly every step.
 **/
struct Position2D final {
    glm::vec2 value{0.0F};
};

/**
 * The position alpha of the way from one to the other. ecs::interpolated() uses it to draw
 * the position between two simulation steps.
 **/
Position2D interpolate(const Position2D& from, const Position2D& to, float alpha);

};  // namespace v3d::ecs::component
