/*
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 */

#pragma once

#include <moya/libmoya/kernel/light/Emission.h>
#include <moya/libmoya/kernel/light/Light.h>
#include <moya/libmoya/kernel/light/LightSample.h>
#include <moya/libmoya/kernel/light/SphericalEllipse.h>
#include <moya/libmoya/kernel/light/SphericalRectangle.h>
#include <moya/libmoya/kernel/Qualifiers.h>
#include <moya/libmoya/kernel/surface/Frame.h>

#include <cmath>
#include <cstdint>

#include <glm/geometric.hpp>
#include <glm/mat3x3.hpp>
#include <glm/mat4x4.hpp>
#include <glm/matrix.hpp>
#include <glm/vec2.hpp>
#include <glm/vec3.hpp>
#include <glm/vec4.hpp>

namespace v3d::moya::kernel::light {

/*
 * Each light is sampled from a point by the solid angle it subtends where a method exists for
 * its shape as it stands in the world, and by area otherwise. A rect whose edges stay at right
 * angles is a spherical rectangle, a disk that stays round is a spherical ellipse, and a sphere
 * that stays round is the cone it subtends from outside it. Anything else is sampled uniformly by
 * its area, and its density converted to solid angle. A spherical rectangle or ellipse below
 * kSmallestSolidAngle is sampled by area too, since its solid angle loses precision there.
 */

/** How a light is sampled from a point. **/
enum class Method : std::uint8_t {
    Area,
    Rectangle,
    Ellipse,
    Cone
};

/** The least solid angle a spherical rectangle or ellipse is sampled in, in steradians. **/
inline constexpr float kSmallestSolidAngle = 1.0e-3F;

/** Below this sine squared of its half angle, a cone is sampled by its small-angle expansion. **/
inline constexpr float kNarrowCone = 0.00068523F;

/** How far two directions may be from a right angle, or two lengths apart, relatively. **/
inline constexpr float kSquareness = 1.0e-4F;

/** A whole turn, in radians. **/
inline constexpr float kTurn = 6.28318530717958647692F;

/** Whether two vectors are at right angles. **/
MOYA_KERNEL inline bool square(const glm::vec3 & first, const glm::vec3 & second) {
    return std::fabs(glm::dot(first, second)) <= kSquareness * glm::length(first) * glm::length(second);
}

/** Whether two lengths are the same. **/
MOYA_KERNEL inline bool alike(float first, float second) {
    return std::fabs(first - second) <= kSquareness * std::fmax(first, second);
}

/** The cofactor matrix of a linear map, which turns a normal and scales an area by its length. **/
MOYA_KERNEL inline glm::mat3 cofactor(const glm::mat3 & map) {
    return glm::mat3 { glm::cross(map[1], map[2]), glm::cross(map[2], map[0]), glm::cross(map[0], map[1]) };
}

/** A light's area in its prototype's space. **/
MOYA_KERNEL inline float localArea(const Light & light) {
    const float first = light.size.x;
    switch (light.shape) {
        case Shape::Rect:
            return first * light.size.y;
        case Shape::Disk:
            return 0.5F * kTurn * first * first;
        case Shape::Sphere:
            return 2.0F * kTurn * first * first;
        case Shape::Cylinder:
            return kTurn * first * light.size.y;
    }
    return 0.0F;
}

/** A point of a light's shape in its prototype's space, drawn uniformly by area. **/
MOYA_KERNEL inline glm::vec3 areaPoint(const Light & light, const glm::vec2 & u) {
    const float first = light.size.x;
    const float angle = kTurn * u.y;
    switch (light.shape) {
        case Shape::Rect:
            return glm::vec3 { (u.x - 0.5F) * first, (u.y - 0.5F) * light.size.y, 0.0F };
        case Shape::Disk: {
            const float out = first * std::sqrt(u.x);
            return glm::vec3 { out * std::cos(angle), out * std::sin(angle), 0.0F };
        }
        case Shape::Sphere: {
            const float z = 1.0F - (2.0F * u.x);
            const float out = std::sqrt(std::fmax(0.0F, 1.0F - (z * z)));
            return glm::vec3 { out * std::cos(angle), out * std::sin(angle), z } * first;
        }
        case Shape::Cylinder:
            return glm::vec3 { first * std::cos(angle), first * std::sin(angle), (u.x - 0.5F) * light.size.y };
    }
    return glm::vec3 { 0.0F };
}

/** Whether a point lies where a light emits to: before a rect's or disk's -z face, or anywhere. **/
MOYA_KERNEL inline bool facing(const Light & light, const glm::mat4 & toWorld, const glm::vec3 & from) {
    if (light.shape != Shape::Rect && light.shape != Shape::Disk) {
        return true;
    }
    return (glm::inverse(toWorld) * glm::vec4(from, 1.0F)).z < 0.0F;
}

/** How a light as it stands in the world is sampled from a point. **/
MOYA_KERNEL inline Method methodOf(const Light & light, const glm::mat4 & toWorld, const glm::vec3 & from) {
    const glm::mat3 map { toWorld };
    switch (light.shape) {
        case Shape::Rect:
            return square(map[0], map[1]) ? Method::Rectangle : Method::Area;
        case Shape::Disk:
            return square(map[0], map[1]) && alike(glm::length(map[0]), glm::length(map[1])) ? Method::Ellipse : Method::Area;
        case Shape::Sphere: {
            const float length = glm::length(map[0]);
            const bool round = square(map[0], map[1]) && square(map[1], map[2]) && square(map[0], map[2]) && alike(length, glm::length(map[1])) &&
                alike(length, glm::length(map[2]));
            return round && glm::length(from - glm::vec3(toWorld[3])) > light.size.x * length ? Method::Cone : Method::Area;
        }
        case Shape::Cylinder:
            break;
    }
    return Method::Area;
}

/** A rect light as a point sees it. **/
MOYA_KERNEL inline SphericalRectangle rectangleOf(const Light & light, const glm::mat4 & toWorld, const glm::vec3 & from) {
    const glm::mat3 map { toWorld };
    const glm::vec3 corner { toWorld * glm::vec4 { -0.5F * light.size.x, -0.5F * light.size.y, 0.0F, 1.0F } };
    return SphericalRectangle::seen(from, corner, map[0] * light.size.x, map[1] * light.size.y);
}

/** A disk light as a point sees it. **/
MOYA_KERNEL inline SphericalEllipse ellipseOf(const Light & light, const glm::mat4 & toWorld, const glm::vec3 & from) {
    const glm::mat3 map { toWorld };
    return SphericalEllipse::fromDisk(from, glm::vec3(toWorld[3]), glm::normalize(glm::cross(map[0], map[1])), light.size.x * glm::length(map[0]));
}

/** One less the cosine of the half angle of the cone a sphere subtends, from the sine squared. **/
MOYA_KERNEL inline float coneRise(float sineSquared) {
    if (sineSquared < kNarrowCone) {
        return 0.5F * sineSquared;
    }
    return 1.0F - std::sqrt(std::fmax(0.0F, 1.0F - sineSquared));
}

/**
 * A point of a sphere light that faces a point outside it, drawn uniformly in the cone it
 * subtends, as pbrt-v4 draws it, with the small-angle expansion for a narrow cone.
 **/
MOYA_KERNEL inline LightSample coneSample(const Light & light, const glm::mat4 & toWorld, const glm::vec3 & from, const glm::vec2 & u) {
    const glm::vec3 centre { toWorld[3] };
    const float radius = light.size.x * glm::length(glm::vec3(toWorld[0]));
    const glm::vec3 toCentre = centre - from;
    const float distance = glm::length(toCentre);
    const float sine = radius / distance;
    const float sineSquared = sine * sine;
    const float rise = coneRise(sineSquared);
    float cosine = 1.0F - (u.x * rise);
    float thetaSquared = 1.0F - (cosine * cosine);
    if (sineSquared < kNarrowCone) {
        thetaSquared = sineSquared * u.x;
        cosine = std::sqrt(1.0F - thetaSquared);
    }
    // the angle at the centre between the direction back to the point and the point drawn
    const float cosAlpha = (thetaSquared / sine) + (cosine * std::sqrt(std::fmax(0.0F, 1.0F - (thetaSquared / sineSquared))));
    const float sinAlpha = std::sqrt(std::fmax(0.0F, 1.0F - (cosAlpha * cosAlpha)));
    const float phi = kTurn * u.y;
    const surface::Frame frame = surface::Frame::around(toCentre / distance);
    const glm::vec3 outward = -frame.toWorld(glm::vec3 { sinAlpha * std::cos(phi), sinAlpha * std::sin(phi), cosAlpha });
    LightSample out;
    out.point = centre + (outward * radius);
    out.incoming = glm::normalize(out.point - from);
    out.density = 1.0F / (kTurn * rise);
    return out;
}

/**
 * The density by solid angle, seen from a point, of a point of a light's shape drawn uniformly
 * by area: its distance squared over the area in the world and the cosine there.
 **/
MOYA_KERNEL inline float areaDensity(const Light & light, const glm::mat4 & toWorld, const glm::vec3 & from, const glm::vec3 & point) {
    const glm::vec3 local { glm::inverse(toWorld) * glm::vec4(point, 1.0F) };
    const glm::vec3 normal = emittingNormal(light, local);
    const float length = glm::length(normal);
    if (!(length > 0.0F)) {
        return 0.0F;
    }
    const glm::vec3 turned = cofactor(glm::mat3(toWorld)) * (normal / length);
    const float stretch = glm::length(turned);
    const glm::vec3 toPoint = point - from;
    const float squared = glm::dot(toPoint, toPoint);
    const float cosine = std::fabs(glm::dot(turned / stretch, toPoint)) / std::sqrt(squared);
    const float area = localArea(light) * stretch;
    if (!(cosine > 0.0F) || !(area > 0.0F)) {
        return 0.0F;
    }
    return squared / (cosine * area);
}

/** A point of a light's shape drawn uniformly by area, with its density by solid angle. **/
MOYA_KERNEL inline LightSample areaSample(const Light & light, const glm::mat4 & toWorld, const glm::vec3 & from, const glm::vec2 & u) {
    LightSample out;
    out.point = glm::vec3 { toWorld * glm::vec4(areaPoint(light, u), 1.0F) };
    const glm::vec3 toPoint = out.point - from;
    const float squared = glm::dot(toPoint, toPoint);
    if (!(squared > 0.0F)) {
        return out;
    }
    out.incoming = toPoint / std::sqrt(squared);
    out.density = areaDensity(light, toWorld, from, out.point);
    return out;
}

/**
 * A point of a light's shape drawn from two random numbers for a point that sees it, as the
 * methods above say. A rect or disk light draws nothing for a point behind its emitting face.
 **/
MOYA_KERNEL inline LightSample sample(const Light & light, const glm::mat4 & toWorld, const glm::vec3 & from, const glm::vec2 & u) {
    if (!facing(light, toWorld, from)) {
        return LightSample {};
    }
    switch (methodOf(light, toWorld, from)) {
        case Method::Rectangle: {
            const SphericalRectangle seen = rectangleOf(light, toWorld, from);
            if (!(seen.solidAngle >= kSmallestSolidAngle)) {
                break;
            }
            LightSample out;
            out.point = seen.sample(u);
            out.incoming = glm::normalize(out.point - from);
            out.density = 1.0F / seen.solidAngle;
            return out;
        }
        case Method::Ellipse: {
            const SphericalEllipse seen = ellipseOf(light, toWorld, from);
            if (!(seen.solidAngle() >= kSmallestSolidAngle)) {
                break;
            }
            const glm::vec3 centre { toWorld[3] };
            const glm::vec3 normal = glm::cross(glm::vec3(toWorld[0]), glm::vec3(toWorld[1]));
            LightSample out;
            out.incoming = glm::normalize(seen.sample(u));
            out.point = from + (out.incoming * (glm::dot(centre - from, normal) / glm::dot(out.incoming, normal)));
            out.density = 1.0F / seen.solidAngle();
            return out;
        }
        case Method::Cone:
            return coneSample(light, toWorld, from, u);
        case Method::Area:
            break;
    }
    return areaSample(light, toWorld, from, u);
}

/**
 * The density by solid angle with which sample() draws a point of a light's shape from a point:
 * zero for a point sample() never draws, such as one on the far side of a sphere.
 **/
MOYA_KERNEL inline float density(const Light & light, const glm::mat4 & toWorld, const glm::vec3 & from, const glm::vec3 & point) {
    if (!facing(light, toWorld, from)) {
        return 0.0F;
    }
    switch (methodOf(light, toWorld, from)) {
        case Method::Rectangle: {
            const float solidAngle = rectangleOf(light, toWorld, from).solidAngle;
            if (solidAngle >= kSmallestSolidAngle) {
                return 1.0F / solidAngle;
            }
            break;
        }
        case Method::Ellipse: {
            const float solidAngle = ellipseOf(light, toWorld, from).solidAngle();
            if (solidAngle >= kSmallestSolidAngle) {
                return 1.0F / solidAngle;
            }
            break;
        }
        case Method::Cone: {
            const glm::vec3 centre { toWorld[3] };
            if (!(glm::dot(point - centre, from - point) > 0.0F)) {
                return 0.0F;
            }
            const float radius = light.size.x * glm::length(glm::vec3(toWorld[0]));
            const glm::vec3 toCentre = centre - from;
            return 1.0F / (kTurn * coneRise((radius * radius) / glm::dot(toCentre, toCentre)));
        }
        case Method::Area:
            break;
    }
    return areaDensity(light, toWorld, from, point);
}

};  // namespace v3d::moya::kernel::light
