/*
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 */

#pragma once

#include <moya/libmoya/kernel/light/LightSample.h>
#include <moya/libmoya/kernel/light/Light.h>
#include <moya/libmoya/kernel/Qualifiers.h>

#include <cmath>

#include <glm/geometric.hpp>
#include <glm/vec2.hpp>
#include <glm/vec3.hpp>

namespace v3d::moya::kernel::light::spherical {

/*
 * A triangle of a mesh light is sampled from a point uniformly in the solid angle it subtends, by
 * Arvo's method as pbrt-v4 gives it. Below kLeastTriangleAngle its solid angle loses precision,
 * and above kMostTriangleAngle the method does, so it is sampled uniformly by its area there and
 * its density converted to solid angle. A triangle emits from the side its normal faces, and draws
 * nothing for a point behind it. faceTowards() turns the normal of a triangle that emits from both
 * faces.
 */

/** The solid angles, in steradians, between which a triangle is sampled in solid angle. **/
inline constexpr float kLeastTriangleAngle = 3.0e-4F;
inline constexpr float kMostTriangleAngle = 6.22F;

/** The angle between two unit vectors, kept precise near zero and near half a turn. **/
MOYA_KERNEL inline float angleBetween(const glm::vec3 & first, const glm::vec3 & second) {
    const auto halfAngle = [](float sine) {
        return std::asin(sine > 1.0F ? 1.0F : sine);
    };
    if (glm::dot(first, second) < 0.0F) {
        return kHalfTurn - (2.0F * halfAngle(0.5F * glm::length(first + second)));
    }
    return 2.0F * halfAngle(0.5F * glm::length(second - first));
}

/** A vector less its part along a unit direction. **/
MOYA_KERNEL inline glm::vec3 without(const glm::vec3 & vector, const glm::vec3 & along) {
    return vector - (along * glm::dot(vector, along));
}

/**
 * The solid angle a triangle subtends from a point, by Van Oosterom and Strackee's formula, or
 * zero for a point in its plane.
 **/
MOYA_KERNEL inline float triangleAngle(const glm::vec3 (&corners)[3], const glm::vec3 & from) {
    const glm::vec3 first = glm::normalize(corners[0] - from);
    const glm::vec3 second = glm::normalize(corners[1] - from);
    const glm::vec3 third = glm::normalize(corners[2] - from);
    const float below = 1.0F + glm::dot(first, second) + glm::dot(first, third) + glm::dot(second, third);
    return std::fabs(2.0F * std::atan2(glm::dot(first, glm::cross(second, third)), below));
}

/** Whether a triangle is sampled from a point in the solid angle it subtends. **/
MOYA_KERNEL inline bool bySolidAngle(float solidAngle) {
    return solidAngle >= kLeastTriangleAngle && solidAngle <= kMostTriangleAngle;
}

/** The weights of a triangle's corners for a point drawn uniformly by area, as pbrt-v4 has it. **/
MOYA_KERNEL inline glm::vec3 areaWeights(const glm::vec2 & u) {
    float first = 0.0F;
    float second = 0.0F;
    if (u.x < u.y) {
        first = 0.5F * u.x;
        second = u.y - first;
    } else {
        second = 0.5F * u.y;
        first = u.x - second;
    }
    return glm::vec3 { first, second, 1.0F - first - second };
}

/**
 * The direction to a point of a spherical triangle, from the unit directions to its corners, drawn
 * uniformly in its solid angle by Arvo's method. The area swept from its first corner is drawn
 * first, which fixes a point along the arc from its first corner to its third. The direction is
 * then drawn along the arc from its second corner to that point.
 **/
MOYA_KERNEL inline glm::vec3 arvo(const glm::vec3 & first, const glm::vec3 & second, const glm::vec3 & third, const glm::vec2 & u) {
    const glm::vec3 acrossAb = glm::normalize(glm::cross(first, second));
    const glm::vec3 acrossBc = glm::normalize(glm::cross(second, third));
    const glm::vec3 acrossCa = glm::normalize(glm::cross(third, first));
    const float alpha = angleBetween(acrossAb, -acrossCa);
    const float beta = angleBetween(acrossBc, -acrossAb);
    const float gamma = angleBetween(acrossCa, -acrossBc);
    // the area swept, plus half a turn
    const float swept = kHalfTurn + (u.x * (alpha + beta + gamma - kHalfTurn));
    const float cosAlpha = std::cos(alpha);
    const float sinAlpha = std::sin(alpha);
    const float sinPhi = (std::sin(swept) * cosAlpha) - (std::cos(swept) * sinAlpha);
    const float cosPhi = (std::cos(swept) * cosAlpha) + (std::sin(swept) * sinAlpha);
    const float k1 = cosPhi + cosAlpha;
    const float k2 = sinPhi - (sinAlpha * glm::dot(first, second));
    float cosB = (k2 + (((k2 * cosPhi) - (k1 * sinPhi)) * cosAlpha)) / (((k2 * sinPhi) + (k1 * cosPhi)) * sinAlpha);
    cosB = std::fmin(1.0F, std::fmax(-1.0F, cosB));
    const float sinB = std::sqrt(std::fmax(0.0F, 1.0F - (cosB * cosB)));
    const glm::vec3 arc = (first * cosB) + (glm::normalize(without(third, first)) * sinB);
    const float cosTheta = 1.0F - (u.y * (1.0F - glm::dot(arc, second)));
    const float sinTheta = std::sqrt(std::fmax(0.0F, 1.0F - (cosTheta * cosTheta)));
    return (second * cosTheta) + (glm::normalize(without(arc, second)) * sinTheta);
}

/** Where a ray from a point along a direction meets a triangle's plane, or the point itself. **/
MOYA_KERNEL inline glm::vec3 onPlane(const glm::vec3 (&corners)[3], const glm::vec3 & normal, const glm::vec3 & from, const glm::vec3 & direction) {
    const float across = glm::dot(direction, normal);
    if (across == 0.0F) {
        return from;
    }
    return from + (direction * (glm::dot(corners[0] - from, normal) / across));
}

/**
 * The density by solid angle, seen from a point, of a point of a triangle drawn uniformly by
 * area: its distance squared over the area and the cosine there.
 **/
// NOLINTNEXTLINE(bugprone-easily-swappable-parameters) - the point it is seen from, then the point
MOYA_KERNEL inline float triangleAreaDensity(const glm::vec3 (&corners)[3], const glm::vec3 & normal, const glm::vec3 & from, const glm::vec3 & point) {
    const float area = 0.5F * glm::length(glm::cross(corners[1] - corners[0], corners[2] - corners[0]));
    const glm::vec3 toPoint = point - from;
    const float squared = glm::dot(toPoint, toPoint);
    const float cosine = std::fabs(glm::dot(normal, toPoint)) / std::sqrt(squared);
    if (!(area > 0.0F) || !(cosine > 0.0F)) {
        return 0.0F;
    }
    return squared / (cosine * area);
}

/** Whether a point lies on the side of a triangle its unit normal faces. **/
MOYA_KERNEL inline bool before(const glm::vec3 (&corners)[3], const glm::vec3 & normal, const glm::vec3 & from) {
    return glm::dot(from - corners[0], normal) > 0.0F;
}

/**
 * Turns the unit normal of a triangle that emits from both faces to the side a point lies on, so
 * the point sees the face that emits towards it. A triangle that emits from one face keeps it.
 **/
MOYA_KERNEL inline void faceTowards(bool twoSided, const glm::vec3 (&corners)[3], const glm::vec3 & from, glm::vec3 & normal) {
    if (twoSided && glm::dot(from - corners[0], normal) < 0.0F) {
        normal = -normal;
    }
}

/**
 * A point of a triangle in the world drawn from two random numbers for a point that sees it, with
 * its density by solid angle. normal is the unit normal of the side it emits from.
 **/
MOYA_KERNEL inline LightSample triangleSample(const glm::vec3 (&corners)[3], const glm::vec3 & normal, const glm::vec3 & from, const glm::vec2 & u) {
    if (!before(corners, normal, from)) {
        return LightSample {};
    }
    LightSample out;
    const float solidAngle = triangleAngle(corners, from);
    if (bySolidAngle(solidAngle)) {
        out.incoming = glm::normalize(arvo(glm::normalize(corners[0] - from), glm::normalize(corners[1] - from), glm::normalize(corners[2] - from), u));
        out.point = onPlane(corners, normal, from, out.incoming);
        out.density = 1.0F / solidAngle;
        return out;
    }
    const glm::vec3 weights = areaWeights(u);
    out.point = (corners[0] * weights.x) + (corners[1] * weights.y) + (corners[2] * weights.z);
    const glm::vec3 toPoint = out.point - from;
    const float squared = glm::dot(toPoint, toPoint);
    if (!(squared > 0.0F)) {
        return LightSample {};
    }
    out.incoming = toPoint / std::sqrt(squared);
    out.density = triangleAreaDensity(corners, normal, from, out.point);
    return out;
}

/** The density by solid angle with which triangleSample() draws a point of a triangle. **/
// NOLINTNEXTLINE(bugprone-easily-swappable-parameters) - the point it is seen from, then the point
MOYA_KERNEL inline float triangleDensity(const glm::vec3 (&corners)[3], const glm::vec3 & normal, const glm::vec3 & from, const glm::vec3 & point) {
    if (!before(corners, normal, from)) {
        return 0.0F;
    }
    const float solidAngle = triangleAngle(corners, from);
    if (bySolidAngle(solidAngle)) {
        return 1.0F / solidAngle;
    }
    return triangleAreaDensity(corners, normal, from, point);
}

};  // namespace v3d::moya::kernel::light::spherical
