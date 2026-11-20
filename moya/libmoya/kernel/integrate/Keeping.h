/*
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 */

#pragma once

#include <moya/libmoya/kernel/geometry/Hit.h>
#include <moya/libmoya/kernel/geometry/Ray.h>
#include <moya/libmoya/kernel/integrate/Opacity.h>
#include <moya/libmoya/kernel/Qualifiers.h>
#include <moya/libmoya/kernel/sample/ZSobol.h>
#include <moya/libmoya/kernel/surface/Surfaces.h>
#include <moya/libmoya/kernel/texture/Texels.h>

#include <cstdint>

#include <glm/mat4x4.hpp>
#include <glm/vec2.hpp>

namespace v3d::moya::kernel::integrate {

/**
 * The filter of a camera or an indirect ray: it keeps a hit with a chance of the surface's
 * opacity, and passes through it otherwise. The chance is decided by a number hashed from the
 * sampler's draw at a dimension and the primitive, so the decision is the same on every run. It
 * keeps every hit on a boundary, which the path passes by itself as it enters or leaves the medium.
 **/
template <texture::Texels X>
struct Keeping {
    const surface::Surfaces & surfaces;
    const X & textures;
    geometry::Ray ray;
    const sample::ZSobol & sampler;
    glm::uvec2 pixel { 0 };
    std::uint32_t sample { 0 };
    std::uint32_t dimension { 0 };

    MOYA_KERNEL bool counts(const geometry::Hit & hit) const {
        if (boundary(hit)) {
            return true;
        }
        const float opacity = opacityAt(surfaces, textures, hit, ray);
        if (!(opacity < 1.0F)) {
            return true;
        }
        return hashed(sampler.get1D(pixel, sample, dimension), hit) < opacity;
    }

    /** Whether a hit is on a boundary. **/
    MOYA_KERNEL bool boundary(const geometry::Hit & hit) const {
        std::uint32_t material = surface::kNoMaterial;
        glm::mat4 toWorld { 1.0F };
        std::uint32_t placed = surface::kGroup;
        return surfaces.chainAt(hit.instances, hit.depth, hit.time, toWorld, placed, material) && material != surface::kNoMaterial &&
            surfaces.materials[material].boundary != 0U;
    }
};

};  // namespace v3d::moya::kernel::integrate
