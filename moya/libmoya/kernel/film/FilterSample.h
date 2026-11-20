/*
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 */

#pragma once

#include <glm/vec2.hpp>

namespace v3d::moya::kernel::film {

/** A camera sample's offset from its pixel's centre, in pixels, and the weight it carries. **/
struct FilterSample {
    glm::vec2 offset { 0.0F };
    float weight { 0.0F };
};

};  // namespace v3d::moya::kernel::film
