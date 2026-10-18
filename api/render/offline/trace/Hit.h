/**
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 **/

#pragma once

#include <glm/vec3.hpp>

#include "Primitive.h"

namespace v3d::render::offline::trace {

/**
 * Where a ray met a primitive, and everything a shader reads there.
 **/
class Hit final {
 public:
    const Primitive* primitive = nullptr;
    float distance = 0.0F;
    /** SL's P, in world space, which is a hit's current space. **/
    glm::vec3 point = glm::vec3(0.0F);
    /**
     * SL's N and Ng: the shading normal, and the geometric normal. A triangle's geometric
     * normal is its plane. A sphere's is the same as its shading normal.
     **/
    glm::vec3 normal = glm::vec3(0.0F);
    glm::vec3 geometric = glm::vec3(0.0F);
    /** SL's I, the direction the surface was seen along. **/
    glm::vec3 incident = glm::vec3(0.0F);
    /**
     * SL's Cs at the hit: the primitive's colour, or a triangle's corner colours weighted as
     * its normals are.
     **/
    glm::vec3 colour = glm::vec3(0.0F);
    /**
     * The surface parameters: a sphere's u and v, and a triangle's barycentric weights,
     * which stand in for them.
     **/
    float u = 0.0F;
    float v = 0.0F;
    /** SL's s and t: a sphere's u and v, and a triangle's "st" at the hit. **/
    float s = 0.0F;
    float t = 0.0F;
};

};  // namespace v3d::render::offline::trace
