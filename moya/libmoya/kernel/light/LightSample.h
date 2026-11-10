/*
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 */

#pragma once

#include <moya/libmoya/kernel/Qualifiers.h>

#include <glm/vec3.hpp>

namespace v3d::moya::kernel::light {

/**
 * A point drawn on a light's shape from a point that sees it: the point in the world, the unit
 * direction to it, and the density it was drawn with, by solid angle. A density of zero means
 * nothing was drawn.
 **/
struct LightSample {
    glm::vec3 point { 0.0F };
    glm::vec3 incoming { 0.0F, 0.0F, 1.0F };
    float density { 0.0F };

    /** Whether a point was drawn. **/
    MOYA_KERNEL bool found() const {
        return density > 0.0F;
    }
};

};  // namespace v3d::moya::kernel::light
