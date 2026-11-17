/*
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 */

#pragma once

#include <moya/libmoya/kernel/bsdf/Albedos.h>
#include <moya/libmoya/kernel/bsdf/Bsdf.h>
#include <moya/libmoya/kernel/bsdf/Closure.h>
#include <moya/libmoya/kernel/bsdf/scattering/F82.h>
#include <moya/libmoya/kernel/bsdf/scattering/Ggx.h>
#include <moya/libmoya/kernel/bsdf/Layering.h>
#include <moya/libmoya/kernel/bsdf/Model.h>
#include <moya/libmoya/kernel/program/Run.h>
#include <moya/libmoya/kernel/Qualifiers.h>
#include <moya/libmoya/kernel/spectrum/Rgb.h>
#include <moya/libmoya/kernel/spectrum/RgbTable.h>
#include <moya/libmoya/kernel/spectrum/Spectrum.h>
#include <moya/libmoya/kernel/spectrum/Wavelengths.h>
#include <moya/libmoya/kernel/surface/Material.h>
#include <moya/libmoya/kernel/surface/MaterialClosure.h>
#include <moya/libmoya/kernel/surface/Normal.h>
#include <moya/libmoya/kernel/surface/OpenPbr.h>
#include <moya/libmoya/kernel/surface/Primvar.h>
#include <moya/libmoya/kernel/surface/ShadingPoint.h>
#include <moya/libmoya/kernel/surface/Surfaces.h>
#include <moya/libmoya/kernel/texture/Texels.h>
#include <moya/libmoya/kernel/texture/Untextured.h>

#include <cmath>
#include <cstdint>

#include <glm/geometric.hpp>
#include <glm/vec3.hpp>

namespace v3d::moya::kernel::bsdf {

/**
 * The BSDF at a shading point, at a path's wavelengths: the closures its material names, or one
 * diffuse closure for a surface with no material. The surface's colour is the geometry's `Cs`
 * primvar, which overrides its material's as RI's does, or its material's surface node's `Cs`,
 * or a mid grey. The colour is in ACEScg, and becomes a reflectance through the RGB-to-spectrum
 * table. A diffuse closure's or a mirror's weight is that reflectance times its scale. An
 * Oren-Nayar lobe's weight is its scale, its colour that reflectance and its roughness its
 * material closure's. A conductor's weight is its scale, and it reflects that reflectance at
 * normal incidence, tinted towards grazing by its F82 tint. A dielectric's weight is its scale,
 * and it takes its index and whether it is thin-walled. A microfacet lobe reads the albedo
 * tables.
 *
 * outside is the index of refraction of the medium on the side the surface's normal faces. A
 * dielectric's index is taken relative to it.
 *
 * A layered material's closures are OpenPBR's, as Layering adds them for light leaving along
 * outgoing, of the parameters its program gives. The geometry's `Cs` primvar overrides its base
 * colour. The program reads its images through a device's texture access. The coat works in the
 * frame about the coat's normal and tangent, and every other closure in the frame about the
 * surface's, each as surface::frameAbout() makes it from the tangent frame Surfaces gives the
 * point.
 **/
template <texture::Texels X>
MOYA_KERNEL Bsdf atSurface(const surface::Surfaces & surfaces, const X & textures, const surface::ShadingPoint & point, const spectrum::RgbTable & table,
    const Albedos & albedos, const spectrum::SampledWavelengths & wavelengths, const glm::vec3 & outgoing, float outside = 1.0F) {
    const surface::Material none;
    const surface::Material & material = point.material != surface::kNoMaterial ? surfaces.materials[point.material] : none;
    glm::vec3 colour { 0.5F };
    float primvar[3] {};
    const bool painted = surfaces.read(point, surface::kColour, primvar, 3U);
    if (painted) {
        colour = glm::vec3 { primvar[0], primvar[1], primvar[2] };
    } else if (material.coloured != 0U) {
        colour = material.colour;
    }
    Bsdf out;
    out.albedos = albedos;
    out.wavelengths = wavelengths;
    out.frame = point.frame;
    out.coatFrame = point.frame;
    out.geometric = point.geometric;
    out.outside = outside;
    if (material.layered != 0U) {
        surface::OpenPbr layers = material.surface;
        if (material.program.count != 0U) {
            layers = program::evaluate(surfaces, textures, point, material.program);
            layers.thinWalled = material.surface.thinWalled;
        }
        if (painted) {
            layers.baseColour = colour;
        }
        const surface::Frame tangents = surfaces.tangentFrame(point);
        out.frame = surface::frameAbout(tangents, layers.geometryNormal, layers.geometryTangent, point.geometric, outgoing);
        out.coatFrame = surface::frameAbout(tangents, layers.geometryCoatNormal, layers.geometryCoatTangent, point.geometric, outgoing);
        Layering::add(out, layers, table, wavelengths, std::fabs(glm::dot(outgoing, out.frame.normal)), std::fabs(glm::dot(outgoing, out.coatFrame.normal)));
        return out;
    }
    const spectrum::SampledSpectrum reflectance = spectrum::reflectance(table, colour, wavelengths);
    for (std::uint32_t i = 0; i < material.count && i < surface::kMostMaterialClosures; ++i) {
        const surface::MaterialClosure & named = material.closures[i];
        if (named.model != Model::Conductor && named.model != Model::Dielectric && named.model != Model::OrenNayar) {
            out.add(named.model, reflectance * named.scale);
            continue;
        }
        Closure closure;
        closure.model = named.model;
        closure.weight = spectrum::SampledSpectrum(named.scale);
        closure.alpha = scattering::Ggx::alphas(named.roughness, named.anisotropy);
        if (named.model == Model::OrenNayar) {
            closure.colour = reflectance;
            closure.roughness = named.roughness;
        } else if (named.model == Model::Conductor) {
            closure.fresnel = scattering::F82 { .normal = reflectance, .tint = spectrum::reflectance(table, named.tint, wavelengths) };
        } else {
            closure.ior = named.ior / outside;
            closure.thin = named.thin;
        }
        out.add(closure);
    }
    return out;
}

/** The BSDF at a shading point with no textures, so each image gives its default. **/
MOYA_KERNEL inline Bsdf atSurface(const surface::Surfaces & surfaces, const surface::ShadingPoint & point, const spectrum::RgbTable & table,
    const Albedos & albedos, const spectrum::SampledWavelengths & wavelengths, const glm::vec3 & outgoing) {
    return atSurface(surfaces, texture::Untextured {}, point, table, albedos, wavelengths, outgoing);
}

};  // namespace v3d::moya::kernel::bsdf
