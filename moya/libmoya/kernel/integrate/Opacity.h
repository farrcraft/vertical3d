/*
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 */

#pragma once

#include <moya/libmoya/kernel/geometry/Hit.h>
#include <moya/libmoya/kernel/geometry/Ray.h>
#include <moya/libmoya/kernel/program/Run.h>
#include <moya/libmoya/kernel/Qualifiers.h>
#include <moya/libmoya/kernel/sample/Hash.h>
#include <moya/libmoya/kernel/surface/Material.h>
#include <moya/libmoya/kernel/surface/ShadingPoint.h>
#include <moya/libmoya/kernel/surface/Surfaces.h>
#include <moya/libmoya/kernel/texture/Texels.h>

#include <cmath>
#include <cstdint>

namespace v3d::moya::kernel::integrate {

/** How far a hash is shifted down to leave the 24 bits a float in [0, 1) holds. **/
inline constexpr std::uint32_t kHashToFloat = 40;

/** One over two to the 24th, which turns 24 bits into a float in [0, 1). **/
inline constexpr float kFloatStep = 1.0F / 16777216.0F;

/** Two to the 24th, which turns a draw in [0, 1) into 24 bits. **/
inline constexpr float kFloatSteps = 16777216.0F;

/**
 * The opacity of the surface a ray met, from zero where it is cut away to one. A surface whose
 * material is not cut out is opaque, and a boundary is clear. A cut-out material's program gives
 * it at the hit, reading its images at their finest level, and otherwise the opacity the material
 * holds gives it.
 **/
template <texture::Texels X>
MOYA_KERNEL float opacityAt(const surface::Surfaces & surfaces, const X & textures, const geometry::Hit & hit, const geometry::Ray & ray) {
    const surface::ShadingPoint point = surfaces.at(hit, ray);
    if (!point.found() || point.material == surface::kNoMaterial) {
        return 1.0F;
    }
    const surface::Material & material = surfaces.materials[point.material];
    if (material.boundary != 0U) {
        return 0.0F;
    }
    if (!material.cutOut()) {
        return 1.0F;
    }
    const float opacity = material.programmedOpacity != 0U ? program::evaluate(surfaces, textures, point, material.program).geometryOpacity : material.surface.geometryOpacity;
    return std::fmin(1.0F, std::fmax(0.0F, opacity));
}

/**
 * A number in [0, 1) for a hit, from a draw of the sample and the primitive the hit met, with the
 * chain of instances that placed it. Two surfaces along one ray take numbers apart from each
 * other, and a primitive met twice takes the same one.
 **/
MOYA_KERNEL inline float hashed(float draw, const geometry::Hit & hit) {
    const auto bits = static_cast<std::uint64_t>(draw * kFloatSteps);
    std::uint64_t value = sample::mix(bits ^ (static_cast<std::uint64_t>(hit.primitive) << 32U));
    for (std::uint32_t level = 0; level < hit.depth && level < geometry::kInstanceDepth; ++level) {
        value = sample::mix(value ^ hit.instances[level]);
    }
    return static_cast<float>(value >> kHashToFloat) * kFloatStep;
}

};  // namespace v3d::moya::kernel::integrate
