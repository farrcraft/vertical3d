/**
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 **/

#include "Sphere.h"

#include <api/render/offline/Units.h>
#include <api/render/offline/rib/Requests.h>
#include <api/type/geometry/AABBox.h>
#include <api/type/geometry/Line.h>
#include <api/type/geometry/Ray.h>

#include <algorithm>
#include <cmath>
#include <limits>

#include <glm/common.hpp>
#include <glm/geometric.hpp>
#include <glm/mat3x3.hpp>
#include <glm/matrix.hpp>

#include "Hit.h"
#include "Intersection.h"
#include "Pose.h"
#include "Primitive.h"

namespace v3d::render::offline::trace {

Sphere::Sphere(const v3d::render::offline::rib::Sphere & shape, const glm::mat4x4 & placement, const glm::vec3 & colour) :
    Primitive(colour), radius_(shape.radius),
    // clamped to the absolute radius, so a negative radius still gives a valid range. A
    // sphere with a negative radius is never hit
    zmin_(std::clamp(std::min(shape.zmin, shape.zmax), -std::fabs(shape.radius), std::fabs(shape.radius))),
    zmax_(std::clamp(std::max(shape.zmin, shape.zmax), -std::fabs(shape.radius), std::fabs(shape.radius))),
    thetamax_(std::clamp(shape.thetamax, 0.0F, v3d::render::offline::rib::Sphere::FULL_THETAMAX) * PI / HALF_TURN_DEGREES),
    toObject_(glm::inverse(placement)) {
}

bool Sphere::intersects(const v3d::type::geometry::Line & line, float from, float* along, glm::vec3* point) const {
    if (!(radius_ > 0.0F)) {
        return false;
    }
    // an affine map keeps a line's parameter, so a root here is a distance in the caller's
    // units without being carried back
    const glm::vec3 origin(toObject_ * glm::vec4(line.origin, 1.0F));
    const glm::vec3 direction = glm::mat3(toObject_) * line.direction;
    const float quadratic = glm::dot(direction, direction);
    const float linear = 2.0F * glm::dot(origin, direction);
    const float constant = glm::dot(origin, origin) - (radius_ * radius_);
    const float discriminant = (linear * linear) - (4.0F * quadratic * constant);
    if (quadratic <= 0.0F || discriminant < 0.0F) {
        return false;
    }
    const float root = std::sqrt(discriminant);
    // the nearer root first, and the far one when the near is behind or cut away: a ray
    // from inside, or one through the open top of a cut sphere, meets the inside
    const float roots[2] = { (-linear - root) / (2.0F * quadratic), (-linear + root) / (2.0F * quadratic) };
    for (const float t : roots) {
        if (t <= from) {
            continue;
        }
        const glm::vec3 position = origin + direction * t;
        if (position.z < zmin_ || position.z > zmax_) {
            continue;
        }
        float phi = std::atan2(position.y, position.x);
        if (phi < 0.0F) {
            phi += 2.0F * PI;
        }
        if (phi > thetamax_) {
            continue;
        }
        *along = t;
        *point = position;
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
    if (phi < 0.0F) {
        phi += 2.0F * PI;
    }
    const float low = std::asin(zmin_ / radius_);
    const float high = std::asin(zmax_ / radius_);
    const float latitude = std::asin(std::clamp(point.z / radius_, -1.0F, 1.0F));
    return glm::vec2(thetamax_ > 0.0F ? phi / thetamax_ : 0.0F,
        high > low ? (latitude - low) / (high - low) : 0.0F);
}

bool Sphere::intersect(const v3d::type::geometry::Ray & ray, float from, const Pose & pose,
    Intersection* found) const {
    // taken back unnormalised, so the parameter along it is still the ray's distance
    const glm::vec3 origin = pose.backward == nullptr ? ray.origin() :
        glm::vec3(*pose.backward * glm::vec4(ray.origin(), 1.0F));
    const glm::vec3 direction = pose.backward == nullptr ? ray.direction() : glm::mat3(*pose.backward) * ray.direction();
    return intersects({.origin = origin, .direction = direction}, from, &found->distance, &found->point);
}

bool Sphere::bound(v3d::type::geometry::AABBox* box) const {
    const float size = std::fabs(radius_);
    v3d::type::geometry::AABBox own;
    own.extents(glm::vec3(-size, -size, zmin_), glm::vec3(size, size, zmax_));
    glm::vec3 corners[8];
    own.vertices(corners);
    // every corner, because under a rotation any of them can hold the extreme on an axis
    const glm::mat4x4 placement = glm::inverse(toObject_);
    glm::vec3 low(std::numeric_limits<float>::max());
    glm::vec3 high(std::numeric_limits<float>::lowest());
    for (const glm::vec3 & corner : corners) {
        const glm::vec3 placed(placement * glm::vec4(corner, 1.0F));
        low = glm::min(low, placed);
        high = glm::max(high, placed);
    }
    box->extents(low, high);
    return true;
}

bool Sphere::at(const glm::vec2 & parameters, Hit* hit) const {
    if (!(radius_ > 0.0F)) {
        return false;
    }
    const float low = std::asin(zmin_ / radius_);
    const float high = std::asin(zmax_ / radius_);
    const float phi = parameters.x * thetamax_;
    const float latitude = low + (parameters.y * (high - low));
    Intersection found;
    found.point = radius_ * glm::vec3(std::cos(latitude) * std::cos(phi), std::cos(latitude) * std::sin(phi),
        std::sin(latitude));
    describe(found, hit);
    hit->point = glm::vec3(glm::inverse(toObject_) * glm::vec4(found.point, 1.0F));
    return true;
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
