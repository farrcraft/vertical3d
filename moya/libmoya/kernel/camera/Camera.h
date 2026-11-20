/*
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 */

#pragma once

#include <moya/libmoya/kernel/geometry/Cone.h>
#include <moya/libmoya/kernel/geometry/Ray.h>
#include <moya/libmoya/kernel/Qualifiers.h>

#include <cmath>

#include <glm/geometric.hpp>
#include <glm/mat4x4.hpp>
#include <glm/matrix.hpp>
#include <glm/vec2.hpp>
#include <glm/vec3.hpp>
#include <glm/vec4.hpp>

/** The camera rays are drawn from. **/
namespace v3d::moya::kernel::camera {

/**
 * A camera as kernel code reads it: RI's camera, looking down +z of its own space with x to the
 * right and y up.
 *
 * A raster position, in pixels from the image's top left, maps onto the screen window, which
 * spans the image. A perspective camera's screen is the plane z = 1 scaled by the tangent of half
 * its field of view, so a window of -1 to 1 spans the field of view. An orthographic camera's
 * screen is the plane z = 0, in camera space's own units.
 **/
struct Camera {
    /** Camera space to world space. **/
    glm::mat4 toWorld { 1.0F };
    bool perspective { false };

    /** The tangent of half the field of view, for a perspective camera. **/
    float tanHalfFov { 1.0F };

    /** The screen window: left, right, bottom and top. **/
    glm::vec4 screen { -1.0F, 1.0F, -1.0F, 1.0F };

    /** The image's size in pixels. **/
    glm::vec2 resolution { 1.0F };

    /** Clipping, as depths along camera space's z. **/
    float near { 0.0F };
    float far { 1.0F };

    /** The lens's radius, zero for a pinhole, and the depth that is in focus. **/
    float lensRadius { 0.0F };
    float focalDistance { 1.0F };

    /** The times the shutter opens and closes. **/
    float open { 0.0F };
    float close { 0.0F };

    /**
     * The ray through a raster position, from a point on the lens drawn from lens in [0, 1)^2,
     * at a time drawn from time in [0, 1) over the shutter.
     *
     * The ray's direction has a z of one in camera space, so a distance along it is the depth of
     * the point it reaches, and its interval is the clipping planes'.
     **/
    // NOLINTNEXTLINE(bugprone-easily-swappable-parameters) - the raster position, then the lens's
    MOYA_KERNEL geometry::Ray ray(const glm::vec2 & raster, const glm::vec2 & lens, float time) const {
        const float sx = screen.x + ((raster.x / resolution.x) * (screen.y - screen.x));
        const float sy = screen.w - ((raster.y / resolution.y) * (screen.w - screen.z));
        glm::vec3 origin { 0.0F };
        glm::vec3 direction { 0.0F, 0.0F, 1.0F };
        if (perspective) {
            direction = glm::vec3 { sx * tanHalfFov, sy * tanHalfFov, 1.0F };
            if (lensRadius > 0.0F) {
                const glm::vec3 focus = direction * focalDistance;
                const glm::vec2 disc = concentric(lens) * lensRadius;
                origin = glm::vec3 { disc.x, disc.y, 0.0F };
                direction = (focus - origin) / focalDistance;
            }
        } else {
            origin = glm::vec3 { sx, sy, 0.0F };
        }
        geometry::Ray out;
        out.origin = glm::vec3(toWorld * glm::vec4(origin, 1.0F));
        out.direction = glm::vec3(toWorld * glm::vec4(direction, 0.0F));
        out.start = near;
        out.end = far;
        out.time = open + (time * (close - open));
        return out;
    }

    /**
     * The raster position a point in the world is seen at, the inverse of ray()'s through a
     * pinhole. A point at a depth of zero or behind a perspective camera has no such position, and
     * gives the screen window's centre.
     **/
    MOYA_KERNEL glm::vec2 raster(const glm::vec3 & world) const {
        const glm::vec3 seen = glm::vec3(glm::inverse(toWorld) * glm::vec4(world, 1.0F));
        glm::vec2 onScreen { seen.x, seen.y };
        if (perspective) {
            if (!(seen.z > 0.0F)) {
                onScreen = glm::vec2 { (screen.x + screen.y) * 0.5F, (screen.z + screen.w) * 0.5F };
            } else {
                onScreen /= seen.z * tanHalfFov;
            }
        }
        return glm::vec2 { ((onScreen.x - screen.x) / (screen.y - screen.x)) * resolution.x, ((screen.w - onScreen.y) / (screen.w - screen.z)) * resolution.y };
    }

    /**
     * The cone of a ray this camera gave, as wide as a pixel where the ray meets the screen's
     * depth. A pinhole's cone starts at no width and spreads by a pixel's width over a unit of
     * depth. A lens's starts as wide as the lens and narrows to a pixel's width at the depth in
     * focus. An orthographic camera's is a pixel wide and does not spread. A pixel's width is the
     * screen window's over the image's width in pixels.
     **/
    MOYA_KERNEL geometry::Cone cone(const geometry::Ray & ray) const {
        const float scale = glm::length(glm::vec3(toWorld[0]));
        const float pixel = (screen.y - screen.x) / resolution.x * scale;
        geometry::Cone out;
        if (!perspective) {
            out.width = pixel;
            return out;
        }
        // a direction has a depth of one in camera space, so its length is distance per depth
        const float length = glm::length(ray.direction);
        out.spread = pixel * tanHalfFov / length;
        if (lensRadius > 0.0F) {
            out.width = 2.0F * lensRadius * scale;
            out.spread -= out.width / (focalDistance * length);
        }
        return out;
    }

    /**
     * A point on the unit disc for a point of the unit square, by Shirley and Chiu's concentric
     * mapping, which keeps neighbouring points near each other. The square's centre is the
     * disc's.
     **/
    MOYA_KERNEL static glm::vec2 concentric(const glm::vec2 & u) {
        const glm::vec2 offset = (u * 2.0F) - 1.0F;
        if (offset.x == 0.0F && offset.y == 0.0F) {
            return glm::vec2 { 0.0F };
        }
        // an eighth of a turn, in radians
        constexpr float kEighth = 0.785398163F;
        if (std::fabs(offset.x) > std::fabs(offset.y)) {
            const float theta = kEighth * (offset.y / offset.x);
            return glm::vec2 { std::cos(theta), std::sin(theta) } * offset.x;
        }
        const float theta = (2.0F * kEighth) - (kEighth * (offset.x / offset.y));
        return glm::vec2 { std::cos(theta), std::sin(theta) } * offset.y;
    }
};

};  // namespace v3d::moya::kernel::camera
