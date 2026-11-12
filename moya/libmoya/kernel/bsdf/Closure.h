/*
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 */

#pragma once

#include <moya/libmoya/kernel/bsdf/scattering/F82.h>
#include <moya/libmoya/kernel/bsdf/scattering/Ggx.h>
#include <moya/libmoya/kernel/bsdf/scattering/ThinFilm.h>
#include <moya/libmoya/kernel/bsdf/Lobe.h>
#include <moya/libmoya/kernel/bsdf/Model.h>
#include <moya/libmoya/kernel/Qualifiers.h>
#include <moya/libmoya/kernel/spectrum/Spectrum.h>

#include <glm/vec2.hpp>

namespace v3d::moya::kernel::bsdf {

/** How far from one a dielectric's index lies before it bends light at all. **/
inline constexpr float kLeastBend = 1.0e-3F;

/**
 * One lobe of a BSDF: its model, its weight at the path's wavelengths, the estimate of its albedo
 * that sampling picks it by, and what its model reads. A microfacet model reads its alphas along
 * the tangent and the bitangent, and a conductor its Fresnel factor. A dielectric reads the index
 * of refraction of the side the shading normal points away from, the outside's being one, and
 * whether it is a thin slab with the same medium on both sides. Its reflection is tinted by its
 * colour and its transmission by its transmitted colour. A specular reflection reads its index
 * and its colour. An Oren-Nayar lobe reads its colour as its reflectance, and its roughness, and
 * fuzz reads its colour and its roughness. A conductor's or a specular reflection's Fresnel factor
 * is mixed towards a thin film's by the film's weight.
 **/
struct Closure {
    Model model { Model::Diffuse };
    spectrum::SampledSpectrum weight { 0.0F };
    float albedo { 0.0F };
    glm::vec2 alpha { 0.5F, 0.5F };
    scattering::F82 fresnel;
    float ior { 1.5F };
    bool thin { false };
    spectrum::SampledSpectrum colour { 1.0F };
    spectrum::SampledSpectrum transmitted { 1.0F };
    float roughness { 0.0F };
    scattering::ThinFilm film;
    float filmWeight { 0.0F };

    /** Whether a thin film changes its Fresnel factor. **/
    MOYA_KERNEL bool filmed() const {
        return filmWeight > 0.0F && film.present();
    }

    /**
     * Whether it scatters into a single direction for each outgoing one, or two for a dielectric.
     * It does so as a mirror, as a microfacet lobe too smooth to have microfacets, or as a
     * dielectric whose index is so near one that light passes straight through. Such a closure
     * has no value and no density for any direction it is given, and only its sample finds its
     * light.
     **/
    MOYA_KERNEL bool singular() const {
        switch (model) {
            case Model::Mirror:
                return true;
            case Model::Conductor:
            case Model::Specular:
                return scattering::Ggx::smooth(alpha);
            case Model::Dielectric:
                return scattering::Ggx::smooth(alpha) || !(ior > 1.0F + kLeastBend);
            case Model::Diffuse:
            case Model::OrenNayar:
            case Model::Fuzz:
                break;
        }
        return false;
    }

    /** The kind of bounce its sample makes: one that crosses the surface is a transmission. **/
    MOYA_KERNEL Lobe lobe(bool crossed) const {
        if (crossed) {
            return Lobe::Transmission;
        }
        if (singular()) {
            return Lobe::Specular;
        }
        return model == Model::Diffuse || model == Model::OrenNayar || model == Model::Fuzz ? Lobe::Diffuse : Lobe::Glossy;
    }
};

};  // namespace v3d::moya::kernel::bsdf
