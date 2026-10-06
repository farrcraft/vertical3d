/**
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 **/

#pragma once

#include <glm/vec3.hpp>

namespace v3d::render::offline::trace {

/**
 * Where a ray met a primitive, as far as the primitive needs to describe the hit afterwards.
 **/
class Intersection final {
 public:
    /** Along the ray, in the ray's own units. **/
    float distance = 0.0f;
    /** A triangle's barycentric weights. **/
    float u = 0.0f;
    float v = 0.0f;
    /** A sphere's point in its own space. **/
    glm::vec3 point = glm::vec3(0.0f);
};

};  // namespace v3d::render::offline::trace
