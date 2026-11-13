/*
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 */

#pragma once

#include <moya/libmoya/kernel/geometry/Hit.h>
#include <moya/libmoya/kernel/geometry/Ray.h>
#include <moya/libmoya/kernel/integrate/Opacity.h>
#include <moya/libmoya/kernel/Qualifiers.h>
#include <moya/libmoya/kernel/surface/Surfaces.h>
#include <moya/libmoya/kernel/texture/Texels.h>

namespace v3d::moya::kernel::integrate {

/**
 * The filter of a shadow ray: each surface it meets lets one minus its opacity through, and the
 * share it carries past them all is passing. A hit counts once nothing passes.
 **/
template <texture::Texels X>
struct Passing {
    const surface::Surfaces & surfaces;
    const X & textures;
    geometry::Ray ray;
    float passing { 1.0F };

    MOYA_KERNEL bool counts(const geometry::Hit & hit) {
        passing *= 1.0F - opacityAt(surfaces, textures, hit, ray);
        return !(passing > 0.0F);
    }
};

};  // namespace v3d::moya::kernel::integrate
