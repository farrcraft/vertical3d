/*
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 */

#pragma once

#include <moya/libmoya/kernel/bsdf/Bsdf.h>
#include <moya/libmoya/kernel/Qualifiers.h>
#include <moya/libmoya/kernel/spectrum/Rgb.h>
#include <moya/libmoya/kernel/spectrum/RgbTable.h>
#include <moya/libmoya/kernel/spectrum/Wavelengths.h>
#include <moya/libmoya/kernel/surface/Material.h>
#include <moya/libmoya/kernel/surface/Primvar.h>
#include <moya/libmoya/kernel/surface/ShadingPoint.h>
#include <moya/libmoya/kernel/surface/Surfaces.h>

#include <glm/vec3.hpp>

namespace v3d::moya::kernel::bsdf {

/**
 * The BSDF at a shading point, at a path's wavelengths. Every surface is diffuse. Its colour is
 * the geometry's `Cs` primvar, which overrides its material's as RI's does, or its material's
 * surface node's `Cs`, or a mid grey. The colour is in ACEScg, and becomes a reflectance through
 * the RGB-to-spectrum table.
 **/
MOYA_KERNEL inline Bsdf atSurface(const surface::Surfaces & surfaces, const surface::ShadingPoint & point, const spectrum::RgbTable & table,
    const spectrum::SampledWavelengths & wavelengths) {
    glm::vec3 colour { 0.5F };
    float primvar[3] {};
    if (surfaces.read(point, surface::kColour, primvar, 3U)) {
        colour = glm::vec3 { primvar[0], primvar[1], primvar[2] };
    } else if (point.material != surface::kNoMaterial && surfaces.materials[point.material].coloured != 0U) {
        colour = surfaces.materials[point.material].colour;
    }
    Bsdf out;
    out.model = Model::Diffuse;
    out.diffuse.reflectance = spectrum::reflectance(table, colour, wavelengths);
    out.frame = point.frame;
    out.geometric = point.geometric;
    return out;
}

};  // namespace v3d::moya::kernel::bsdf
