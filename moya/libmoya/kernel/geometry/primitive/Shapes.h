/*
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 */

#pragma once

#include <moya/libmoya/kernel/geometry/Between.h>
#include <moya/libmoya/kernel/geometry/Motion.h>
#include <moya/libmoya/kernel/geometry/primitive/Polynomial.h>
#include <moya/libmoya/kernel/geometry/primitive/Quadratic.h>
#include <moya/libmoya/kernel/geometry/primitive/Quadric.h>
#include <moya/libmoya/kernel/geometry/primitive/ShapeHit.h>
#include <moya/libmoya/kernel/geometry/Ray.h>
#include <moya/libmoya/kernel/geometry/Sampled.h>
#include <moya/libmoya/kernel/Qualifiers.h>

#include <cmath>
#include <cstdint>

#include <glm/geometric.hpp>
#include <glm/vec2.hpp>
#include <glm/vec3.hpp>

/**
 * The primitives a ray can meet, and the tests that find where it meets each. Every primitive but
 * the triangle is intersected here on every device.
 **/
namespace v3d::moya::kernel::geometry::primitive {

/** What a geometry's primitives are: triangles, round points, flat points, or one quadric. **/
enum class Form : std::uint8_t {
    Triangles,
    RoundPoints,
    FlatPoints,
    Quadric
};

/** A quadric between two of its samples, each dimension interpolated linearly. **/
MOYA_KERNEL inline Quadric mixed(const Quadric & from, const Quadric & to, float along) {
    const auto mix = [along](float start, float finish) {
        return start + ((finish - start) * along);
    };
    Quadric out = from;
    out.radius = mix(from.radius, to.radius);
    out.minorRadius = mix(from.minorRadius, to.minorRadius);
    out.zmin = mix(from.zmin, to.zmin);
    out.zmax = mix(from.zmax, to.zmax);
    out.height = mix(from.height, to.height);
    out.phimin = mix(from.phimin, to.phimin);
    out.phimax = mix(from.phimax, to.phimax);
    out.point1 = from.point1 + ((to.point1 - from.point1) * along);
    out.point2 = from.point2 + ((to.point2 - from.point2) * along);
    out.thetamax = mix(from.thetamax, to.thetamax);
    return out;
}

/** A quadric at a time, between the two of its samples that bracket it. **/
// NOLINTNEXTLINE(bugprone-easily-swappable-parameters) - the samples, then the time
MOYA_KERNEL inline Quadric sampledQuadric(const Sampled & held, const Quadric * quadrics, const float * times, float time) {
    const Between at = between(
        [&](std::uint32_t sample) {
            return times[held.times + sample];
        },
        held.samples, time);
    const Quadric & from = quadrics[held.first + at.at];
    if (at.along == 0.0F) {
        return from;
    }
    return mixed(from, quadrics[held.first + at.at + 1U], at.along);
}

/** The angle of a direction in the plane, from +x towards +y, in [0, a whole turn). **/
MOYA_KERNEL inline float angleOf(float x, float y) {
    const float angle = std::atan2(y, x);
    return angle < 0.0F ? angle + kWholeTurn : angle;
}

/**
 * The roots of a quadratic, nearest first, computed so that neither loses precision to the
 * other. Gives how many there are: none, or two, which may be equal.
 **/
MOYA_KERNEL inline std::uint32_t quadraticRoots(const Quadratic & terms, float & near, float & far) {
    if (terms.squared == 0.0F) {
        if (terms.linear == 0.0F) {
            return 0;
        }
        near = far = -terms.constant / terms.linear;
        return 2;
    }
    const float discriminant = (terms.linear * terms.linear) - (4.0F * terms.squared * terms.constant);
    if (discriminant < 0.0F) {
        return 0;
    }
    const float half = -0.5F * (terms.linear + std::copysign(std::sqrt(discriminant), terms.linear));
    near = half / terms.squared;
    far = half != 0.0F ? terms.constant / half : near;
    if (near > far) {
        const float swap = near;
        near = far;
        far = swap;
    }
    return 2;
}

/**
 * The two distances at which a ray crosses a sphere of a radius about a centre, nearest first, or
 * false when it passes by. The discriminant is taken from the point on the ray nearest the centre,
 * so a ray from far off does not lose the radius to rounding.
 **/
MOYA_KERNEL inline bool sphereCrossings(const glm::vec3 & centre, float radius, const Ray & ray, float & near, float & far) {
    const float squared = glm::dot(ray.direction, ray.direction);
    const glm::vec3 offset = ray.origin - centre;
    const float linear = 2.0F * glm::dot(offset, ray.direction);
    const glm::vec3 nearest = offset - (ray.direction * (linear / (2.0F * squared)));
    const float gap = (radius * radius) - glm::dot(nearest, nearest);
    // a gap that is not a number, from a ray of no length, passes by too
    if (gap < 0.0F || squared <= 0.0F || std::isnan(gap)) {
        return false;
    }
    const float constant = glm::dot(offset, offset) - (radius * radius);
    const float half = -0.5F * (linear + std::copysign(std::sqrt(4.0F * squared * gap), linear));
    near = half / squared;
    far = half != 0.0F ? constant / half : near;
    if (near > far) {
        const float swap = near;
        near = far;
        far = swap;
    }
    return true;
}

/**
 * Whether a ray meets a round point, a sphere of a radius about a centre, within [start, end),
 * and where. Its position is zero.
 **/
MOYA_KERNEL inline bool intersectSphere(const glm::vec3 & centre, float radius, const Ray & ray, float start, float end, ShapeHit & hit) {
    float near = 0.0F;
    float far = 0.0F;
    if (!sphereCrossings(centre, radius, ray, near, far)) {
        return false;
    }
    const float distance = near >= start ? near : far;
    if (distance < start || !(distance < end)) {
        return false;
    }
    hit.distance = distance;
    hit.uv = glm::vec2 { 0.0F };
    return true;
}

/**
 * Whether a ray meets a flat point, a disk of a radius about a centre facing the ray, within
 * [start, end), and where. Its position is zero.
 **/
MOYA_KERNEL inline bool intersectFacing(const glm::vec3 & centre, float radius, const Ray & ray, float start, float end, ShapeHit & hit) {
    const float squared = glm::dot(ray.direction, ray.direction);
    if (!(squared > 0.0F)) {
        return false;
    }
    const float distance = glm::dot(centre - ray.origin, ray.direction) / squared;
    const glm::vec3 across = ray.at(distance) - centre;
    if (distance < start || !(distance < end) || glm::dot(across, across) > radius * radius) {
        return false;
    }
    hit.distance = distance;
    hit.uv = glm::vec2 { 0.0F };
    return true;
}

/** Where a value lies between two limits, from 0 to 1, or false outside them. **/
MOYA_KERNEL inline bool within(float value, float first, float second, float & along) {
    const float lower = std::fmin(first, second);
    const float upper = std::fmax(first, second);
    if (value < lower || value > upper) {
        return false;
    }
    along = upper > lower ? (value - lower) / (upper - lower) : 0.0F;
    return true;
}

/** A sphere's v at a point on it: its latitude between those of its cuts. **/
MOYA_KERNEL inline bool sphereProfile(const Quadric & sphere, const glm::vec3 & point, float & v) {
    float unused = 0.0F;
    if (!within(point.z, sphere.zmin, sphere.zmax, unused)) {
        return false;
    }
    const auto latitude = [&](float z) {
        return std::asin(std::fmax(-1.0F, std::fmin(1.0F, z / sphere.radius)));
    };
    return within(latitude(point.z), latitude(sphere.zmin), latitude(sphere.zmax), v);
}

/**
 * A torus's v at a point on it: its angle about the tube, between phimin and phimax. The angle is
 * turned into the whole turn from the lower limit in one step, so any limits end the test. An
 * infinite or NaN limit meets nothing.
 **/
MOYA_KERNEL inline bool torusProfile(const Quadric & torus, const glm::vec3 & point, float & v) {
    const float reach = std::sqrt((point.x * point.x) + (point.y * point.y));
    const float phi = std::atan2(point.z, reach - torus.radius);
    const float lower = std::fmin(torus.phimin, torus.phimax);
    const float span = std::fabs(torus.phimax - torus.phimin);
    float turned = std::fmod(phi - lower, kWholeTurn);
    turned = turned < 0.0F ? turned + kWholeTurn : turned;
    if (!(turned <= span)) {
        return false;
    }
    v = span > 0.0F ? turned / span : 0.0F;
    return true;
}

/**
 * The fraction along a flat hyperboloid's line of a point in its plane: the first place from
 * point1 whose distance from z is the point's, within [0, 1]. A distance the line does not reach
 * gives the nearest place it does.
 **/
MOYA_KERNEL inline float flatAlong(const Quadric & hyperboloid, const glm::vec3 & point) {
    const glm::vec3 along = hyperboloid.point2 - hyperboloid.point1;
    const float spread = (along.x * along.x) + (along.y * along.y);
    const float cross = (hyperboloid.point1.x * along.x) + (hyperboloid.point1.y * along.y);
    // |point1 + along v|^2 in the plane, less the point's distance from z squared
    const Quadratic terms { .squared = spread,
        .linear = 2.0F * cross,
        .constant = (hyperboloid.point1.x * hyperboloid.point1.x) + (hyperboloid.point1.y * hyperboloid.point1.y) - (point.x * point.x) -
                    (point.y * point.y) };
    float near = 0.0F;
    float far = 0.0F;
    if (quadraticRoots(terms, near, far) == 0) {
        // nearer z than the line reaches, which rounding gives at its nearest place
        return spread > 0.0F ? std::fmax(0.0F, std::fmin(1.0F, -cross / spread)) : 0.0F;
    }
    const float first = near >= 0.0F || far < 0.0F ? near : far;
    return std::fmax(0.0F, std::fmin(1.0F, first));
}

/**
 * A hyperboloid's v at a point on it, the fraction along its line, and its angle about z from the
 * line's own angle at that place, where the sweep starts. On a rising line the place is at the
 * point's height, and on a flat one flatAlong() finds it.
 **/
// NOLINTNEXTLINE(bugprone-easily-swappable-parameters) - the angle, then the fraction along
MOYA_KERNEL inline bool hyperboloidProfile(const Quadric & hyperboloid, const glm::vec3 & point, float & theta, float & v) {
    const glm::vec3 along = hyperboloid.point2 - hyperboloid.point1;
    v = along.z != 0.0F ? (point.z - hyperboloid.point1.z) / along.z : flatAlong(hyperboloid, point);
    if (v < 0.0F || v > 1.0F) {
        return false;
    }
    const glm::vec3 line = hyperboloid.point1 + (along * v);
    theta = angleOf(point.x, point.y) - angleOf(line.x, line.y);
    theta = theta < 0.0F ? theta + kWholeTurn : theta;
    return true;
}

/** The position along a quadric's profile of a point on it, or false outside its limits. **/
MOYA_KERNEL inline bool profileOf(const Quadric & quadric, const glm::vec3 & point, float & theta, float & v) {
    switch (quadric.shape) {
        case Shape::Sphere:
            return sphereProfile(quadric, point, v);
        case Shape::Cylinder:
        case Shape::Paraboloid:
            return within(point.z, quadric.zmin, quadric.zmax, v);
        case Shape::Cone:
            v = quadric.height != 0.0F ? point.z / quadric.height : 0.0F;
            return v >= 0.0F && v <= 1.0F;
        case Shape::Hyperboloid:
            return hyperboloidProfile(quadric, point, theta, v);
        case Shape::Disk: {
            const float reach = std::sqrt((point.x * point.x) + (point.y * point.y));
            v = quadric.radius > 0.0F ? 1.0F - (reach / quadric.radius) : 0.0F;
            return reach <= quadric.radius;
        }
        case Shape::Torus:
            return torusProfile(quadric, point, v);
    }
    return false;
}

/**
 * The position on a quadric's surface of a point on it, or false when the point lies outside its
 * sweep or its limits. u runs along the sweep and v along the profile, as RI parameterises them.
 **/
MOYA_KERNEL inline bool quadricPosition(const Quadric & quadric, const glm::vec3 & point, glm::vec2 & uv) {
    float theta = angleOf(point.x, point.y);
    float v = 0.0F;
    if (!profileOf(quadric, point, theta, v) || theta > quadric.thetamax) {
        return false;
    }
    uv = glm::vec2 { quadric.thetamax > 0.0F ? theta / quadric.thetamax : 0.0F, v };
    return true;
}

/**
 * Whether a ray meets a torus within [start, end), and where. The quartic is solved for a ray
 * moved to start where it passes nearest the torus's centre, with a direction of unit length, so
 * its coefficients stay small.
 **/
MOYA_KERNEL inline bool intersectTorus(const Quadric & torus, const Ray & ray, float start, float end, ShapeHit & hit) {
    const float length = glm::length(ray.direction);
    if (!(length > 0.0F)) {
        return false;
    }
    const glm::vec3 direction = ray.direction / length;
    const float shift = -glm::dot(ray.origin, direction);
    const glm::vec3 origin = ray.origin + (direction * shift);
    const float outer = torus.radius + torus.minorRadius;
    const float closest = glm::dot(origin, origin);
    if (closest > outer * outer) {
        return false;
    }
    // (|p|^2 + R^2 - r^2)^2 = 4 R^2 (x^2 + y^2), with |p|^2 = t^2 + |o|^2 since o . d = 0
    const float major = torus.radius * torus.radius;
    const float sum = closest + major - (torus.minorRadius * torus.minorRadius);
    const float flatSquared = (direction.x * direction.x) + (direction.y * direction.y);
    const float flatLinear = 2.0F * ((origin.x * direction.x) + (origin.y * direction.y));
    const float flatConstant = (origin.x * origin.x) + (origin.y * origin.y);
    const float coefficients[kHighestDegree + 1U] = { (sum * sum) - (4.0F * major * flatConstant), -4.0F * major * flatLinear,
        (2.0F * sum) - (4.0F * major * flatSquared), 0.0F, 1.0F };
    // the ray's interval, in the moved ray's distances, within the sphere that holds the torus
    const float reach = std::sqrt(std::fmax(0.0F, (outer * outer) - closest));
    const float lower = std::fmax((start * length) - shift, -reach);
    const float upper = std::fmin((end * length) - shift, reach);
    if (!(lower <= upper)) {
        return false;
    }
    float roots[kHighestDegree] {};
    const std::uint32_t count = rootsIn(coefficients, kHighestDegree, lower, upper, roots);
    for (std::uint32_t root = 0; root < count; ++root) {
        const float distance = (roots[root] + shift) / length;
        glm::vec2 uv { 0.0F };
        if (distance >= start && distance < end && quadricPosition(torus, ray.at(distance), uv)) {
            hit.distance = distance;
            hit.uv = uv;
            return true;
        }
    }
    return false;
}

/**
 * The quadratic in the distance along a ray whose roots are where the ray meets a cone, a
 * cylinder, a hyperboloid whose line rises, or a paraboloid. None for a cone of no height or a
 * paraboloid cut at its apex, which are no surface.
 **/
MOYA_KERNEL inline Quadratic quadricTerms(const Quadric & quadric, const Ray & ray) {
    const glm::vec3 & origin = ray.origin;
    const glm::vec3 & direction = ray.direction;
    // x^2 + y^2 along the ray, which each shape sets equal to its radius at the ray's z
    const Quadratic flat { .squared = (direction.x * direction.x) + (direction.y * direction.y),
        .linear = 2.0F * ((origin.x * direction.x) + (origin.y * direction.y)),
        .constant = (origin.x * origin.x) + (origin.y * origin.y) };
    switch (quadric.shape) {
        case Shape::Cylinder:
            return Quadratic { .squared = flat.squared, .linear = flat.linear, .constant = flat.constant - (quadric.radius * quadric.radius) };
        case Shape::Cone: {
            if (quadric.height == 0.0F) {
                return {};
            }
            // radius (1 - z / height), squared
            const float taper = (quadric.radius / quadric.height) * (quadric.radius / quadric.height);
            const float apex = origin.z - quadric.height;
            return Quadratic { .squared = flat.squared - (taper * direction.z * direction.z),
                .linear = flat.linear - (2.0F * taper * direction.z * apex),
                .constant = flat.constant - (taper * apex * apex) };
        }
        case Shape::Paraboloid: {
            if (quadric.zmax == 0.0F) {
                return {};
            }
            // radius^2 z / zmax
            const float opening = (quadric.radius * quadric.radius) / quadric.zmax;
            return Quadratic { .squared = flat.squared, .linear = flat.linear - (opening * direction.z), .constant = flat.constant - (opening * origin.z) };
        }
        case Shape::Hyperboloid: {
            // |line(v)|^2 in the plane, with v rising linearly in z along the line
            const glm::vec3 along = quadric.point2 - quadric.point1;
            const float from = (origin.z - quadric.point1.z) / along.z;
            const float rise = direction.z / along.z;
            const float base = (quadric.point1.x * quadric.point1.x) + (quadric.point1.y * quadric.point1.y);
            const float cross = (quadric.point1.x * along.x) + (quadric.point1.y * along.y);
            const float spread = (along.x * along.x) + (along.y * along.y);
            return Quadratic { .squared = flat.squared - (spread * rise * rise),
                .linear = flat.linear - (2.0F * ((cross * rise) + (spread * from * rise))),
                .constant = flat.constant - (base + (2.0F * cross * from) + (spread * from * from)) };
        }
        // a sphere, a disk and a torus are not met by a quadratic in the distance
        case Shape::Sphere:
        case Shape::Disk:
        case Shape::Torus:
            break;
    }
    return {};
}

/** Whether a ray meets a sphere cut at zmin and zmax and swept by thetamax, and where. **/
MOYA_KERNEL inline bool intersectCutSphere(const Quadric & sphere, const Ray & ray, float start, float end, ShapeHit & hit) {
    float near = 0.0F;
    float far = 0.0F;
    if (!sphereCrossings(glm::vec3 { 0.0F }, sphere.radius, ray, near, far)) {
        return false;
    }
    // the nearer crossing may lie outside the cut, so the farther is tried too
    const float crossings[2] = { near, far };
    for (const float distance : crossings) {
        glm::vec2 uv { 0.0F };
        if (distance >= start && distance < end && quadricPosition(sphere, ray.at(distance), uv)) {
            hit.distance = distance;
            hit.uv = uv;
            return true;
        }
    }
    return false;
}

/**
 * Whether a ray meets a flat quadric, a disk or a hyperboloid whose line lies in a plane, and
 * where. A flat hyperboloid sweeps a ring between its line's nearest and farthest reach from z.
 **/
MOYA_KERNEL inline bool intersectFlat(const Quadric & quadric, const Ray & ray, float start, float end, ShapeHit & hit) {
    const float plane = quadric.shape == Shape::Disk ? quadric.height : quadric.point1.z;
    if (ray.direction.z == 0.0F) {
        return false;
    }
    const float distance = (plane - ray.origin.z) / ray.direction.z;
    const glm::vec3 point = ray.at(distance);
    glm::vec2 uv { 0.0F };
    if (distance < start || !(distance < end) || !quadricPosition(quadric, point, uv)) {
        return false;
    }
    if (quadric.shape == Shape::Hyperboloid) {
        const glm::vec3 along = quadric.point2 - quadric.point1;
        const float spread = (along.x * along.x) + (along.y * along.y);
        const float onLine =
            spread > 0.0F ? std::fmax(0.0F, std::fmin(1.0F, -((quadric.point1.x * along.x) + (quadric.point1.y * along.y)) / spread)) : 0.0F;
        const glm::vec3 inner = quadric.point1 + (along * onLine);
        const float reach = (point.x * point.x) + (point.y * point.y);
        const float least = (inner.x * inner.x) + (inner.y * inner.y);
        const float most = std::fmax((quadric.point1.x * quadric.point1.x) + (quadric.point1.y * quadric.point1.y),
            (quadric.point2.x * quadric.point2.x) + (quadric.point2.y * quadric.point2.y));
        if (reach < least || reach > most) {
            return false;
        }
    }
    hit.distance = distance;
    hit.uv = uv;
    return true;
}

/**
 * Whether a ray meets a quadric in its own space within [start, end), and where: the nearer of
 * its crossings that lies within the quadric's sweep and limits. Its position is RI's: u along
 * the sweep, and v along the profile.
 **/
MOYA_KERNEL inline bool intersectQuadric(const Quadric & quadric, const Ray & ray, float start, float end, ShapeHit & hit) {
    if (quadric.shape == Shape::Torus) {
        return intersectTorus(quadric, ray, start, end, hit);
    }
    if (quadric.shape == Shape::Sphere) {
        return intersectCutSphere(quadric, ray, start, end, hit);
    }
    if (quadric.shape == Shape::Disk || (quadric.shape == Shape::Hyperboloid && quadric.point1.z == quadric.point2.z)) {
        return intersectFlat(quadric, ray, start, end, hit);
    }
    float near = 0.0F;
    float far = 0.0F;
    if (quadraticRoots(quadricTerms(quadric, ray), near, far) == 0) {
        return false;
    }
    const float crossings[2] = { near, far };
    for (const float distance : crossings) {
        glm::vec2 uv { 0.0F };
        if (distance >= start && distance < end && quadricPosition(quadric, ray.at(distance), uv)) {
            hit.distance = distance;
            hit.uv = uv;
            return true;
        }
    }
    return false;
}

/** A quadric's geometric normal at a point on it, in its own space, not of unit length. **/
MOYA_KERNEL inline glm::vec3 quadricNormal(const Quadric & quadric, const glm::vec3 & point) {
    switch (quadric.shape) {
        case Shape::Sphere:
            return point;
        case Shape::Cylinder:
            return glm::vec3 { point.x, point.y, 0.0F };
        case Shape::Cone: {
            const float taper = (quadric.radius / quadric.height) * (quadric.radius / quadric.height);
            return glm::vec3 { point.x, point.y, -taper * (point.z - quadric.height) };
        }
        case Shape::Paraboloid: {
            const float opening = (quadric.radius * quadric.radius) / quadric.zmax;
            return glm::vec3 { point.x, point.y, -0.5F * opening };
        }
        case Shape::Hyperboloid: {
            const glm::vec3 along = quadric.point2 - quadric.point1;
            if (along.z == 0.0F) {
                return glm::vec3 { 0.0F, 0.0F, 1.0F };
            }
            const float v = (point.z - quadric.point1.z) / along.z;
            const glm::vec3 line = quadric.point1 + (along * v);
            // half the slope of |line(v)|^2 with z
            const float slope = ((line.x * along.x) + (line.y * along.y)) / along.z;
            return glm::vec3 { point.x, point.y, -slope };
        }
        case Shape::Disk:
            return glm::vec3 { 0.0F, 0.0F, 1.0F };
        case Shape::Torus: {
            const float sum = glm::dot(point, point) + (quadric.radius * quadric.radius) - (quadric.minorRadius * quadric.minorRadius);
            const float major = quadric.radius * quadric.radius;
            return glm::vec3 { point.x * (sum - (2.0F * major)), point.y * (sum - (2.0F * major)), point.z * sum };
        }
    }
    return glm::vec3 { 0.0F, 0.0F, 1.0F };
}

};  // namespace v3d::moya::kernel::geometry::primitive
