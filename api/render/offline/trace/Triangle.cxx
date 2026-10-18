/**
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 **/

#include "Triangle.h"

#include <glm/geometric.hpp>
#include <glm/mat3x3.hpp>

#include "Hit.h"

namespace v3d::render::offline::trace {

namespace {

/** The least length of a triangle's edge cross product that gives it a plane. **/
constexpr float SMALLEST_TRIANGLE_AREA = 1.0e-8f;

/**
 * The plane of a triangle, wound the way its corners are. A triangle with no area names no
 * plane and returns zero rather than normalising a zero vector.
 **/
glm::vec3 plane(const glm::vec3 & a, const glm::vec3 & b, const glm::vec3 & c) {
    const glm::vec3 across = glm::cross(b - a, c - a);
    const float area = glm::length(across);
    return area > SMALLEST_TRIANGLE_AREA ? across / area : glm::vec3(0.0f);
}

};  // namespace

Triangle::Triangle(const Corners & corners, const glm::vec3 & colour) :
    Primitive(colour), a_(corners[0]), b_(corners[1]), c_(corners[2]), geometric_(plane(a_, b_, c_)) {
    na_ = geometric_;
    nb_ = geometric_;
    nc_ = geometric_;
}

Triangle::Triangle(const Corners & corners, const glm::vec3 & colour, const Corners & normals) :
    Primitive(colour), a_(corners[0]), b_(corners[1]), c_(corners[2]), na_(normals[0]), nb_(normals[1]), nc_(normals[2]),
    geometric_(plane(a_, b_, c_)) {
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
    // normals cancel give none at all, so the plane is used instead
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

void Triangle::colours(const glm::vec3 & a, const glm::vec3 & b, const glm::vec3 & c) {
    coloured_ = true;
    ca_ = a;
    cb_ = b;
    cc_ = c;
}

glm::vec3 Triangle::colour(float u, float v) const {
    if (!coloured_) {
        return Primitive::colour();
    }
    return ca_ * (1.0f - u - v) + cb_ * u + cc_ * v;
}

bool Triangle::intersect(const v3d::type::geometry::Ray & ray, float from, const Pose & pose,
    Intersection* found) const {
    const v3d::type::geometry::Ray local = pose.backward == nullptr ? ray :
        v3d::type::geometry::Ray({.origin = glm::vec3(*pose.backward * glm::vec4(ray.origin(), 1.0f)), .direction = glm::mat3(*pose.backward) * ray.direction()});
    float distance = 0.0f;
    glm::vec2 weights(0.0f);
    if (!local.intersects(a_, b_, c_, &distance, &weights)) {
        return false;
    }
    if (pose.ahead != nullptr) {
        // a distance along the ray taken back is in the stored pose's units, which a
        // motion that scales does not keep
        const glm::vec3 there(*pose.ahead * glm::vec4(local.origin() + local.direction() * distance, 1.0f));
        distance = glm::dot(there - ray.origin(), ray.direction());
    }
    if (distance <= from) {
        return false;
    }
    found->distance = distance;
    found->u = weights.x;
    found->v = weights.y;
    return true;
}

void Triangle::describe(const Intersection & found, Hit* hit) const {
    hit->normal = shadingNormal(found.u, found.v);
    hit->geometric = geometric_;
    hit->u = found.u;
    hit->v = found.v;
    const glm::vec2 coordinates = st(found.u, found.v);
    hit->s = coordinates.x;
    hit->t = coordinates.y;
    hit->colour = colour(found.u, found.v);
}

};  // namespace v3d::render::offline::trace
