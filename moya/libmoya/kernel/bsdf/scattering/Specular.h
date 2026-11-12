/*
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 */

#pragma once

#include <moya/libmoya/kernel/bsdf/Albedos.h>
#include <moya/libmoya/kernel/bsdf/Closure.h>
#include <moya/libmoya/kernel/bsdf/scattering/Conductor.h>
#include <moya/libmoya/kernel/bsdf/scattering/Diffuse.h>
#include <moya/libmoya/kernel/bsdf/scattering/Fresnel.h>
#include <moya/libmoya/kernel/bsdf/scattering/Ggx.h>
#include <moya/libmoya/kernel/bsdf/scattering/ThinFilm.h>
#include <moya/libmoya/kernel/Qualifiers.h>
#include <moya/libmoya/kernel/spectrum/Spectrum.h>
#include <moya/libmoya/kernel/spectrum/Wavelengths.h>

#include <cmath>

#include <glm/geometric.hpp>
#include <glm/vec3.hpp>

namespace v3d::moya::kernel::bsdf::scattering {

/**
 * A dielectric's reflection alone, as a layer over a base reflects: the specular layer and the
 * coat. It works in a frame whose z is the shading normal, and reflects on whichever side light
 * leaves towards, as though that side faced up. The light it does not reflect is the base's,
 * which the layering scales by one less its albedo.
 *
 * - **A single scattering** is GGX's, by the closure's alphas, times Fresnel's equations at the
 *   angle between outgoing and the microfacet normal, for the closure's index.
 * - **What a single scattering loses** is given back as a conductor's is, its Fresnel factor's mean
 *   being the mean a smooth interface reflects. Both parts are reciprocal.
 * - **The closure's colour tints it**, and a thin film's reflectance at each wavelength is mixed
 *   into its Fresnel factor by the film's weight. The compensation takes no film.
 * - **A sample** draws as a conductor's does.
 **/
struct Specular {
    /** The value for two directions, before the closure's weight. **/
    MOYA_KERNEL static spectrum::SampledSpectrum evaluate(const Closure & closure, const Albedos & albedos, const spectrum::SampledWavelengths & wavelengths,
        glm::vec3 outgoing, glm::vec3 incoming) {
        if (!(outgoing.z * incoming.z > 0.0F)) {
            return spectrum::SampledSpectrum(0.0F);
        }
        if (outgoing.z < 0.0F) {
            outgoing.z = -outgoing.z;
            incoming.z = -incoming.z;
        }
        const Ggx ggx { closure.alpha };
        const glm::vec3 normal = glm::normalize(outgoing + incoming);
        const spectrum::SampledSpectrum single = fresnel(closure, glm::dot(outgoing, normal), wavelengths) * ggx.reflected(outgoing, incoming);
        return closure.colour * (single + spectrum::SampledSpectrum(compensation(albedos, shared(closure), index(closure), outgoing.z, incoming.z)));
    }

    /** What a smooth one reflects along the mirror direction of outgoing. **/
    MOYA_KERNEL static spectrum::SampledSpectrum mirrored(const Closure & closure, const spectrum::SampledWavelengths & wavelengths, const glm::vec3 & outgoing) {
        return closure.colour * fresnel(closure, std::fabs(outgoing.z), wavelengths);
    }

    /** Its Fresnel factor at a cosine, at each wavelength, with its film mixed in. **/
    MOYA_KERNEL static spectrum::SampledSpectrum fresnel(const Closure & closure, float cosine, const spectrum::SampledWavelengths & wavelengths) {
        const spectrum::SampledSpectrum bare(Fresnel::reflected(cosine, index(closure)));
        if (!closure.filmed()) {
            return bare;
        }
        return bare + ((closure.film.overDielectric(cosine, index(closure), wavelengths) - bare) * closure.filmWeight);
    }

    /**
     * Its albedo, white, at outgoing's cosine, for an alpha and an index: the table's reflection
     * alone, and what the compensation gives back. A layer below it is scaled by one less this.
     **/
    // NOLINTNEXTLINE(bugprone-easily-swappable-parameters) - the cosine, the alpha, then the index
    MOYA_KERNEL static float albedo(const Albedos & albedos, float cosine, float alpha, float eta) {
        const float once = albedos.reflectedAt(cosine, alpha, eta);
        const float mean = albedos.averageAt(alpha);
        if (!(mean < 1.0F)) {
            return once;
        }
        return once + ((1.0F - albedos.at(cosine, alpha)) * bounced(albedos, eta, mean));
    }

 private:
    /** The closure's index, no less than one. **/
    MOYA_KERNEL static float index(const Closure & closure) {
        return std::fmax(closure.ior, 1.0F);
    }

    /** The alpha the tables are read at for a closure's two: their geometric mean. **/
    MOYA_KERNEL static float shared(const Closure & closure) {
        return std::sqrt(closure.alpha.x * closure.alpha.y);
    }

    /**
     * The share of what a single scattering loses that the bounces between microfacets return:
     * the mean Fresnel factor squared times the mean albedo, over one less the mean factor times
     * one less the mean albedo.
     **/
    // NOLINTNEXTLINE(bugprone-easily-swappable-parameters) - the index, then the mean albedo
    MOYA_KERNEL static float bounced(const Albedos & albedos, float eta, float mean) {
        const float fresnel = 1.0F - albedos.transmittedAt(eta);
        return fresnel * fresnel * mean / (1.0F - (fresnel * (1.0F - mean)));
    }

    /** The compensation's value for two cosines above the surface. **/
    // NOLINTNEXTLINE(bugprone-easily-swappable-parameters) - the alpha, the index, then the cosines
    MOYA_KERNEL static float compensation(const Albedos & albedos, float alpha, float eta, float outgoing, float incoming) {
        const float mean = albedos.averageAt(alpha);
        if (!(mean < 1.0F)) {
            return 0.0F;
        }
        const float shape = (1.0F - albedos.at(outgoing, alpha)) * (1.0F - albedos.at(incoming, alpha)) * kInversePi / (1.0F - mean);
        return shape * bounced(albedos, eta, mean);
    }
};

};  // namespace v3d::moya::kernel::bsdf::scattering
