/**
 * Vertical3D
 * Copyright(c) 2023 Joshua Farr(josh@farrcraft.com)
 **/

#include "Position2D.h"

#include <glm/common.hpp>

namespace v3d::ecs::component {

Position2D interpolate(const Position2D& from, const Position2D& to, float alpha) {
    return Position2D{glm::mix(from.value, to.value, alpha)};
}

};  // namespace v3d::ecs::component
