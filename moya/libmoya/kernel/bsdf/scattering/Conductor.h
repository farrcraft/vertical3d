/*
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 */

#pragma once

#include <moya/libmoya/kernel/bsdf/Albedos.h>
#include <moya/libmoya/kernel/bsdf/Closure.h>
#include <moya/libmoya/kernel/bsdf/scattering/Diffuse.h>
#include <moya/libmoya/kernel/bsdf/scattering/Ggx.h>
#include <moya/libmoya/kernel/Qualifiers.h>
#include <moya/libmoya/kernel/spectrum/Spectrum.h>
#include <moya/libmoya/kernel/spectrum/Wavelengths.h>

#include <cmath>

#include <glm/geometric.hpp>
#include <glm/vec2.hpp>
#include <glm/vec3.hpp>

namespace v3d::moya::kernel::bsdf::scattering {

/**
 * A metal's reflection, in a frame whose z is the shading normal and whose x is the tangent. It
 * reflects on whichever side light leaves towards, as though that side faced up.
 *
 * - **A single scattering** is GGX's, by the closure's alphas, times its F82 Fresnel factor at the
 *   angle between outgoing and the microfacet normal.
 * - **What a single scattering loses** is given back by Kulla and Conty's compensation. It is a
 *   lobe of one minus each direction's albedo, over pi times one minus the mean albedo. The
 *   albedos are read from the table at the alpha the two alphas share. It is coloured by the
 *   Fresnel factor's mean over the bounces the light makes between microfacets. Both parts are
 *   reciprocal.
 * - **A sample** draws a visible normal with the chance of outgoing's albedo, and reflects about
 *   it. Otherwise it draws by the cosine, as the compensation is shaped.
 **/
struct Conductor {
    /** The value for two directions, before the closure's weight. **/
    MOYA_KERNEL static spectrum::SampledSpectrum evaluate(const Closure & closure, const Albedos & albedos, const spectrum::SampledWavelengths & wavelengths,
        glm::vec3 outgoing, glm::vec3 incoming) {
        if (!(outgoing.z * incoming.z > 0.0F)) {
            return spectrum::SampledSpectrum(0.0F);
        }
        upward(outgoing, incoming);
        const Ggx ggx { closure.alpha };
        const glm::vec3 normal = glm::normalize(outgoing + incoming);
        return (reflectance(closure, glm::dot(outgoing, normal), wavelengths) * ggx.reflected(outgoing, incoming)) +
            compensation(closure, albedos, outgoing.z, incoming.z);
    }

    /** The density sample() draws incoming with, for outgoing. **/
    MOYA_KERNEL static float density(const Closure & closure, const Albedos & albedos, glm::vec3 outgoing, glm::vec3 incoming) {
        if (!(outgoing.z * incoming.z > 0.0F)) {
            return 0.0F;
        }
        upward(outgoing, incoming);
        const Ggx ggx { closure.alpha };
        const float single = share(closure, albedos, outgoing.z);
        return (single * ggx.reflectedDensity(outgoing, incoming)) + ((1.0F - single) * incoming.z * kInversePi);
    }

    /** A direction light comes in from, on outgoing's side, drawn from two random numbers. **/
    MOYA_KERNEL static glm::vec3 sample(const Closure & closure, const Albedos & albedos, const glm::vec3 & outgoing, const glm::vec2 & u) {
        const float side = outgoing.z < 0.0F ? -1.0F : 1.0F;
        const glm::vec3 up { outgoing.x, outgoing.y, outgoing.z * side };
        const float single = share(closure, albedos, up.z);
        glm::vec3 out { 0.0F };
        if (u.x < single) {
            const Ggx ggx { closure.alpha };
            const glm::vec3 normal = ggx.sampleVisible(up, glm::vec2 { u.x / single, u.y });
            out = (2.0F * glm::dot(up, normal) * normal) - up;
        } else {
            out = Diffuse::sample(up, glm::vec2 { (u.x - single) / (1.0F - single), u.y });
        }
        return glm::vec3 { out.x, out.y, out.z * side };
    }

    /** What a smooth conductor reflects along the mirror direction of outgoing. **/
    MOYA_KERNEL static spectrum::SampledSpectrum mirrored(const Closure & closure, const spectrum::SampledWavelengths & wavelengths, const glm::vec3 & outgoing) {
        return reflectance(closure, std::fabs(outgoing.z), wavelengths);
    }

    /**
     * Its Fresnel factor at a cosine, at each wavelength: its F82 factor, with a thin film's
     * reflectance over it mixed in by the film's weight. The compensation takes no film.
     **/
    MOYA_KERNEL static spectrum::SampledSpectrum reflectance(const Closure & closure, float cosine, const spectrum::SampledWavelengths & wavelengths) {
        const spectrum::SampledSpectrum bare = closure.fresnel(cosine);
        if (!closure.filmed()) {
            return bare;
        }
        const auto inside = [&closure](float within) { return closure.fresnel(within); };
        const spectrum::SampledSpectrum filmed = closure.film.overConductor(cosine, closure.fresnel.normal, inside, wavelengths);
        return bare + ((filmed - bare) * closure.filmWeight);
    }

    /** The alpha the table is read at for a closure's two: their geometric mean. **/
    MOYA_KERNEL static float shared(const Closure & closure) {
        return std::sqrt(closure.alpha.x * closure.alpha.y);
    }

    /** The compensation's value for two cosines above the surface. **/
    MOYA_KERNEL static spectrum::SampledSpectrum compensation(const Closure & closure, const Albedos & albedos, float outgoing, float incoming) {
        const float alpha = shared(closure);
        const float mean = albedos.averageAt(alpha);
        if (!(mean < 1.0F)) {
            return spectrum::SampledSpectrum(0.0F);
        }
        const float shape = (1.0F - albedos.at(outgoing, alpha)) * (1.0F - albedos.at(incoming, alpha)) * kInversePi / (1.0F - mean);
        // light that leaves the first microfacet meets more, each time by the Fresnel factor's
        // mean, and each bounce after the first escapes with the mean albedo
        const spectrum::SampledSpectrum fresnel = closure.fresnel.average();
        spectrum::SampledSpectrum out;
        for (int i = 0; i < spectrum::kSamples; ++i) {
            out[i] = shape * fresnel[i] * fresnel[i] * mean / (1.0F - (fresnel[i] * (1.0F - mean)));
        }
        return out;
    }

 private:
    /** Turns two directions on one side to face up. **/
    MOYA_KERNEL static void upward(glm::vec3 & outgoing, glm::vec3 & incoming) {
        if (outgoing.z < 0.0F) {
            outgoing.z = -outgoing.z;
            incoming.z = -incoming.z;
        }
    }

    /** The chance a sample draws a visible normal: outgoing's albedo, from zero to one. **/
    MOYA_KERNEL static float share(const Closure & closure, const Albedos & albedos, float outgoing) {
        return std::fmin(std::fmax(albedos.at(outgoing, shared(closure)), 0.0F), 1.0F);
    }
};

};  // namespace v3d::moya::kernel::bsdf::scattering
