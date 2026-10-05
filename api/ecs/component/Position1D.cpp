/**
 * Vertical3D
 * Copyright(c) 2023 Joshua Farr(josh@farrcraft.com)
 **/

#include "Position1D.h"

#include <glm/common.hpp>

namespace v3d::ecs::component {

Position1D interpolate(const Position1D& from, const Position1D& to, float alpha) {
    return Position1D{glm::mix(from.value, to.value, alpha)};
}

};  // namespace v3d::ecs::component
