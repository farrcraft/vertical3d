/**
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 **/

#include "Sphere.h"

#include <algorithm>
#include <cmath>

#include <glm/geometric.hpp>
#include <glm/mat3x3.hpp>
#include <glm/matrix.hpp>

#include "Hit.h"

namespace v3d::render::offline::trace {

namespace {

const float PI = 3.14159265358979323846f;

};  // namespace

Sphere::Sphere(float radius, float zmin, float zmax, float thetamax, const glm::mat4x4 & placement,
    const glm::vec3 & colour) :
    Primitive(colour), radius_(radius),
    // clamped by the size of the radius, so a negative one is still a valid range; such a
    // sphere is never hit anyway
    zmin_(std::clamp(std::min(zmin, zmax), -std::fabs(radius), std::fabs(radius))),
    zmax_(std::clamp(std::max(zmin, zmax), -std::fabs(radius), std::fabs(radius))),
    thetamax_(std::clamp(thetamax, 0.0f, 360.0f) * PI / 180.0f),
    toObject_(glm::inverse(placement)) {
}

bool Sphere::intersects(const glm::vec3 & origin, const glm::vec3 & direction, float from,
    float* along, glm::vec3* point) const {
    if (!(radius_ > 0.0f)) {
        return false;
    }
    // an affine map keeps a line's parameter, so a root here is a distance in the caller's
    // units without being carried back
    const glm::vec3 o(toObject_ * glm::vec4(origin, 1.0f));
    const glm::vec3 d = glm::mat3(toObject_) * direction;
    const float a = glm::dot(d, d);
    const float b = 2.0f * glm::dot(o, d);
    const float c = glm::dot(o, o) - radius_ * radius_;
    const float discriminant = b * b - 4.0f * a * c;
    if (a <= 0.0f || discriminant < 0.0f) {
        return false;
    }
    const float root = std::sqrt(discriminant);
    // the nearer root first, and the far one when the near is behind or cut away: a ray
    // from inside, or one through the open top of a cut sphere, meets the inside
    const float roots[2] = { (-b - root) / (2.0f * a), (-b + root) / (2.0f * a) };
    for (const float t : roots) {
        if (t <= from) {
            continue;
        }
        const glm::vec3 p = o + d * t;
        if (p.z < zmin_ || p.z > zmax_) {
            continue;
        }
        float phi = std::atan2(p.y, p.x);
        if (phi < 0.0f) {
            phi += 2.0f * PI;
        }
        if (phi > thetamax_) {
            continue;
        }
        *along = t;
        *point = p;
        return true;
    }
    return false;
}

glm::vec3 Sphere::normal(const glm::vec3 & point) const {
    const glm::mat3 normals = glm::transpose(glm::mat3(toObject_));
    return glm::normalize(normals * (point / radius_));
}

glm::vec2 Sphere::parameters(const glm::vec3 & point) const {
    float phi = std::atan2(point.y, point.x);
    if (phi < 0.0f) {
        phi += 2.0f * PI;
    }
    const float low = std::asin(zmin_ / radius_);
    const float high = std::asin(zmax_ / radius_);
    const float latitude = std::asin(std::clamp(point.z / radius_, -1.0f, 1.0f));
    return glm::vec2(thetamax_ > 0.0f ? phi / thetamax_ : 0.0f,
        high > low ? (latitude - low) / (high - low) : 0.0f);
}

bool Sphere::intersect(const v3d::type::geometry::Ray & ray, float from, const Pose & pose,
    Intersection* found) const {
    // taken back unnormalised, so the parameter along it is still the ray's distance
    const glm::vec3 origin = pose.backward == nullptr ? ray.origin() :
        glm::vec3(*pose.backward * glm::vec4(ray.origin(), 1.0f));
    const glm::vec3 direction = pose.backward == nullptr ? ray.direction() : glm::mat3(*pose.backward) * ray.direction();
    return intersects(origin, direction, from, &found->distance, &found->point);
}

void Sphere::describe(const Intersection & found, Hit* hit) const {
    hit->normal = normal(found.point);
    hit->geometric = hit->normal;
    const glm::vec2 surface = parameters(found.point);
    hit->u = surface.x;
    hit->v = surface.y;
    hit->s = surface.x;
    hit->t = surface.y;
}

};  // namespace v3d::render::offline::trace
