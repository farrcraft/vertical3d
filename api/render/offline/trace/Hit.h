/**
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 **/

#pragma once

#include <glm/vec3.hpp>

#include "Primitive.h"

namespace v3d::render::offline::trace {

/**
 * Where a ray met a primitive, and everything a shader is a function of there.
 *
 * A hit's batch is this, one point of it: the same program and the same instructions that
 * run over a grid of a hundred in moya, with a mask one bit wide.
 **/
class Hit final {
 public:
    const Primitive* primitive = nullptr;
    float distance = 0.0f;
    /** SL's P, in world space, which is a hit's current space. **/
    glm::vec3 point = glm::vec3(0.0f);
    /** SL's N and Ng: the interpolated shading normal and the triangle's plane. **/
    glm::vec3 normal = glm::vec3(0.0f);
    glm::vec3 geometric = glm::vec3(0.0f);
    /** SL's I, the direction the surface was seen along. **/
    glm::vec3 incident = glm::vec3(0.0f);
    /**
     * The surface parameters: a sphere's u and v, and a triangle's barycentric weights,
     * which stand in for them.
     **/
    float u = 0.0f;
    float v = 0.0f;
    /** SL's s and t: a sphere's u and v, and a triangle's "st" at the hit. **/
    float s = 0.0f;
    float t = 0.0f;
};

};  // namespace v3d::render::offline::trace
