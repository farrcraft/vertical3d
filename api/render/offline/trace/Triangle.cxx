/**
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 **/

#include "Triangle.h"

#include <glm/geometric.hpp>

namespace v3d::render::offline::trace {

namespace {

/**
 * The plane of a triangle, wound the way its corners are. A triangle with no area names no
 * plane and answers zero rather than a normalised nothing.
 **/
glm::vec3 plane(const glm::vec3 & a, const glm::vec3 & b, const glm::vec3 & c) {
    const glm::vec3 across = glm::cross(b - a, c - a);
    const float area = glm::length(across);
    return area > 1.0e-8f ? across / area : glm::vec3(0.0f);
}

};  // namespace

Triangle::Triangle(const glm::vec3 & a, const glm::vec3 & b, const glm::vec3 & c, const glm::vec3 & colour) :
    Primitive(colour), a_(a), b_(b), c_(c), geometric_(plane(a, b, c)) {
    na_ = geometric_;
    nb_ = geometric_;
    nc_ = geometric_;
}

Triangle::Triangle(const glm::vec3 & a, const glm::vec3 & b, const glm::vec3 & c, const glm::vec3 & colour,
    const glm::vec3 & na, const glm::vec3 & nb, const glm::vec3 & nc) :
    Primitive(colour), a_(a), b_(b), c_(c), na_(na), nb_(nb), nc_(nc), geometric_(plane(a, b, c)) {
}

const glm::vec3 & Triangle::a() const {
    return a_;
}

const glm::vec3 & Triangle::b() const {
    return b_;
}

const glm::vec3 & Triangle::c() const {
    return c_;
}

const glm::vec3 & Triangle::geometricNormal() const {
    return geometric_;
}

glm::vec3 Triangle::shadingNormal(float u, float v) const {
    const glm::vec3 normal = na_ * (1.0f - u - v) + nb_ * u + nc_ * v;
    // interpolating unit normals does not give a unit one back, and three corners whose
    // normals cancel give none at all - the plane is what is left to answer with
    const float length = glm::length(normal);
    return length > 0.0f ? normal / length : geometric_;
}

void Triangle::st(const glm::vec2 & a, const glm::vec2 & b, const glm::vec2 & c) {
    sta_ = a;
    stb_ = b;
    stc_ = c;
}

glm::vec2 Triangle::st(float u, float v) const {
    return sta_ * (1.0f - u - v) + stb_ * u + stc_ * v;
}

};  // namespace v3d::render::offline::trace
