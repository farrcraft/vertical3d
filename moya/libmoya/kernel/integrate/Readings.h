/*
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 */

#pragma once

#include <moya/libmoya/kernel/Array.h>
#include <moya/libmoya/kernel/camera/Camera.h>
#include <moya/libmoya/kernel/film/Pixels.h>
#include <moya/libmoya/kernel/geometry/Hit.h>
#include <moya/libmoya/kernel/geometry/Ray.h>
#include <moya/libmoya/kernel/integrate/FirstHit.h>
#include <moya/libmoya/kernel/Launch.h>
#include <moya/libmoya/kernel/Qualifiers.h>
#include <moya/libmoya/kernel/surface/Primvar.h>
#include <moya/libmoya/kernel/surface/ShadingPoint.h>
#include <moya/libmoya/kernel/surface/Surfaces.h>

#include <cstdint>
#include <limits>

#include <glm/vec2.hpp>
#include <glm/vec3.hpp>

namespace v3d::moya::kernel::integrate {

/**
 * What a camera ray reads for one channel at its first hit, into the channel's components. The
 * point is the hit's shading point, which finds nothing for a light's shape or a ray that met
 * nothing.
 **/
// NOLINTNEXTLINE(readability-function-cognitive-complexity) - one case for each reading
MOYA_KERNEL inline void readAt(const FirstHit & wanted, const camera::Camera & camera, const surface::Surfaces & surfaces, const geometry::Hit & hit,
    const geometry::Ray & ray, const surface::ShadingPoint & point, float * out) {
    for (std::uint32_t i = 0; i < 4U; ++i) {
        out[i] = 0.0F;
    }
    switch (wanted.reading) {
    case Reading::Depth:
        out[0] = hit.found() ? hit.distance : std::numeric_limits<float>::infinity();
        return;
    case Reading::Position:
        if (hit.found()) {
            const glm::vec3 position = ray.at(hit.distance);
            for (glm::length_t i = 0; i < 3; ++i) {
                out[i] = position[i];
            }
        }
        return;
    case Reading::Normal:
        if (point.found()) {
            for (glm::length_t i = 0; i < 3; ++i) {
                out[i] = point.shading[i];
            }
        }
        return;
    case Reading::Texture:
        if (point.found() && !surfaces.read(point, surface::kTexture, out, 2U)) {
            out[0] = point.uv.x;
            out[1] = point.uv.y;
        }
        return;
    case Reading::Instance:
        out[0] = hit.found() && hit.depth != 0U ? static_cast<float>(hit.instances[hit.depth - 1U]) : -1.0F;
        return;
    case Reading::Face:
        out[0] = point.found() && point.face != geometry::kNoPrimitive ? static_cast<float>(point.face) : -1.0F;
        return;
    case Reading::Primvar:
        if (point.found()) {
            surfaces.read(point, wanted.primvar, out, wanted.components);
        }
        return;
    case Reading::Motion:
        if (point.found()) {
            const glm::vec2 moved = camera.raster(surfaces.positionAt(point, hit, camera.close)) - camera.raster(surfaces.positionAt(point, hit, camera.open));
            out[0] = moved.x;
            out[1] = moved.y;
        }
        return;
    case Reading::Albedo:
    case Reading::DenoisingNormal:
        return;
    }
}

/**
 * Adds a camera sample's first hit to each channel the path tracer fills there, by each one's
 * combine. A closest channel keeps the sample of the least depth, and a ray that met nothing is
 * the furthest.
 **/
// NOLINTNEXTLINE(bugprone-easily-swappable-parameters) - the pixel, then the sample's weight
MOYA_KERNEL inline void readFirstHit(const film::Pixels & pixels, Item pixel, float weight, const Array<FirstHit> & wanted, const camera::Camera & camera,
    const surface::Surfaces & surfaces, const geometry::Hit & hit, const geometry::Ray & ray, const surface::ShadingPoint & point) {
    const float key = hit.found() ? hit.distance : std::numeric_limits<float>::infinity();
    for (std::uint32_t i = 0; i < wanted.size; ++i) {
        float value[4];
        readAt(wanted[i], camera, surfaces, hit, ray, point, value);
        pixels.keep(pixel, wanted[i].offset, wanted[i].combine, weight, key, value, wanted[i].components);
    }
}

/**
 * Adds a camera sample's albedo and normal to each channel a denoiser reads, by each one's
 * combine. A closest channel keeps the sample whose surface was nearest along its last ray.
 **/
// NOLINTNEXTLINE(bugprone-easily-swappable-parameters) - the pixel, then the sample's weight
MOYA_KERNEL inline void readFeatures(const film::Pixels & pixels, Item pixel, float weight, const Array<FirstHit> & wanted, float depth,
    const glm::vec3 & albedo, const glm::vec3 & normal) {
    for (std::uint32_t i = 0; i < wanted.size; ++i) {
        const glm::vec3 & read = wanted[i].reading == Reading::Albedo ? albedo : normal;
        const float value[3] = { read.x, read.y, read.z };
        pixels.keep(pixel, wanted[i].offset, wanted[i].combine, weight, depth, value, 3U);
    }
}

};  // namespace v3d::moya::kernel::integrate
