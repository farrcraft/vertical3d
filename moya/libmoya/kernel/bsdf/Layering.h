/*
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 */

#pragma once

#include <moya/libmoya/kernel/bsdf/Bsdf.h>
#include <moya/libmoya/kernel/bsdf/Closure.h>
#include <moya/libmoya/kernel/bsdf/scattering/F82.h>
#include <moya/libmoya/kernel/bsdf/scattering/Fresnel.h>
#include <moya/libmoya/kernel/bsdf/scattering/Fuzz.h>
#include <moya/libmoya/kernel/bsdf/scattering/Ggx.h>
#include <moya/libmoya/kernel/bsdf/Model.h>
#include <moya/libmoya/kernel/bsdf/scattering/Specular.h>
#include <moya/libmoya/kernel/bsdf/scattering/ThinFilm.h>
#include <moya/libmoya/kernel/Qualifiers.h>
#include <moya/libmoya/kernel/spectrum/Rgb.h>
#include <moya/libmoya/kernel/spectrum/RgbTable.h>
#include <moya/libmoya/kernel/spectrum/Spectrum.h>
#include <moya/libmoya/kernel/spectrum/Wavelengths.h>
#include <moya/libmoya/kernel/surface/OpenPbr.h>

#include <cmath>

#include <glm/vec2.hpp>
#include <glm/vec3.hpp>

namespace v3d::moya::kernel::bsdf {

/** The most a modulated specular layer reflects at normal incidence. **/
inline constexpr float kMostNormalReflectance = 0.9999F;

/** Nanometres in a micrometre, the unit OpenPBR gives a thin film's thickness in. **/
inline constexpr float kNanometres = 1000.0F;

/** The least the darkening's denominator is taken as before the darkening is left out. **/
inline constexpr float kLeastDarkening = 1.0e-6F;

/**
 * How OpenPBR Surface's fuzz, coat, base and specular layer become a BSDF's closures, top
 * down. Each layer scales what lies below it by one less its albedo at outgoing's cosine, as a
 * white one reflects it. That keeps the energy of a white surface, though the whole is not
 * reciprocal.
 **/
struct Layering {
    /**
     * Adds the closures of OpenPBR's parameters to a BSDF whose albedo tables are set, for light
     * leaving at a cosine to the shading normal. Colours become spectra through the table.
     *
     * - **The fuzz**, of its weight, tinted by its colour, lies over everything. What lies below
     *   takes one less its weight times its albedo, untinted.
     * - **The coat**, of its weight, is a specular reflection of its own roughness and index. What
     *   lies below takes one less its weight, and its weight times one less its albedo, its colour
     *   and its darkening.
     * - **The coat roughens the layers below it.** Their roughness becomes the fourth root of its
     *   fourth power plus twice the coat's, at most one, mixed in by the coat's weight.
     * - **The metal**, of the base's metalness, is a conductor of the base's colour and the
     *   specular colour as its F82 tint, scaled by the specular weight.
     * - **The rest is a dielectric.** Its index is the specular index with its reflectance at
     *   normal incidence scaled by the specular weight. Of the transmission's weight, it is a
     *   dielectric that transmits, tinted by the specular colour and the transmission colour.
     * - **The opaque rest** is a specular reflection over the diffuse base, its index relative to
     *   the coat's where the coat lies over it. The diffuse base is an Oren-Nayar lobe of the
     *   base's roughness, its colour the base colour times its weight, mixed with the subsurface
     *   colour by the subsurface weight. It takes one less the specular layer's albedo.
     * - **A thin film**, of its weight, lies over the metal and the opaque specular layer, its
     *   index relative to the coat's as the specular layer's is. The layer below the specular
     *   layer takes one less its albedo times its film's reflectance over its bare Fresnel factor,
     *   at outgoing's cosine and each wavelength.
     **/
    MOYA_KERNEL static void add(Bsdf & bsdf, const surface::OpenPbr & surface, const spectrum::RgbTable & table, const spectrum::SampledWavelengths & wavelengths,
        float cosine) {
        const float coat = unit(surface.coatWeight);
        const float metal = unit(surface.baseMetalness);
        const float transmission = unit(surface.transmissionWeight);
        const float coatIor = std::fmax(surface.coatIor, 1.0F);
        const glm::vec3 base = surface.baseWeight * surface.baseColour;
        const spectrum::SampledSpectrum diffuse = spectrum::reflectance(table, mix(base, surface.subsurfaceColour, unit(surface.subsurfaceWeight)), wavelengths);
        const spectrum::SampledSpectrum metallic = spectrum::reflectance(table, base, wavelengths);
        const spectrum::SampledSpectrum specularColour = spectrum::reflectance(table, surface.specularColour, wavelengths);
        float roughness = unit(surface.specularRoughness);
        bsdf.wavelengths = wavelengths;
        spectrum::SampledSpectrum below(1.0F);
        const float fuzz = unit(surface.fuzzWeight);
        if (fuzz > 0.0F) {
            Closure layer;
            layer.model = Model::Fuzz;
            layer.weight = spectrum::SampledSpectrum(fuzz);
            layer.colour = spectrum::reflectance(table, surface.fuzzColour, wavelengths);
            layer.roughness = unit(surface.fuzzRoughness);
            bsdf.add(layer);
            below = spectrum::SampledSpectrum(1.0F - (fuzz * scattering::Fuzz::albedo(cosine, layer.roughness)));
        }
        const scattering::ThinFilm film { .thickness = std::fmax(surface.thinFilmThickness, 0.0F) * kNanometres,
            .ior = relative(std::fmax(surface.thinFilmIor, 1.0F), coatIor, coat) };
        const float filmWeight = unit(surface.thinFilmWeight);
        if (coat > 0.0F) {
            Closure layer;
            layer.model = Model::Specular;
            layer.weight = spectrum::SampledSpectrum(coat);
            layer.alpha = scattering::Ggx::alphas(unit(surface.coatRoughness), unit(surface.coatAnisotropy));
            layer.ior = coatIor;
            bsdf.add(layer);
            const float coatRoughness = unit(surface.coatRoughness);
            const float roughened = std::fmin(1.0F, std::pow(std::pow(roughness, 4.0F) + (2.0F * std::pow(coatRoughness, 4.0F)), 0.25F));
            roughness += (roughened - roughness) * coat;
            const float reflected = scattering::Specular::albedo(bsdf.albedos, cosine, std::sqrt(layer.alpha.x * layer.alpha.y), coatIor);
            const spectrum::SampledSpectrum lowest = mix(diffuse * (1.0F - transmission), metallic, metal);
            const float baseRoughness = roughness + ((1.0F - roughness) * (1.0F - metal) * (1.0F - transmission));
            const spectrum::SampledSpectrum darkened = darkening(bsdf.albedos, lowest, coatIor, baseRoughness, cosine, unit(surface.coatDarkening));
            below *= spectrum::SampledSpectrum(1.0F - coat) + (spectrum::reflectance(table, surface.coatColour, wavelengths) * darkened * (coat * (1.0F - reflected)));
        }
        const glm::vec2 alpha = scattering::Ggx::alphas(roughness, unit(surface.specularAnisotropy));
        const float ior = modulated(std::fmax(surface.specularIor, 1.0F), std::fmax(surface.specularWeight, 0.0F));
        if (metal > 0.0F) {
            Closure layer;
            layer.model = Model::Conductor;
            layer.weight = below * (metal * std::fmax(surface.specularWeight, 0.0F));
            layer.alpha = alpha;
            layer.fresnel = scattering::F82 { .normal = metallic, .tint = specularColour };
            layer.film = film;
            layer.filmWeight = filmWeight;
            bsdf.add(layer);
        }
        const spectrum::SampledSpectrum dielectric = below * (1.0F - metal);
        if (transmission > 0.0F) {
            Closure layer;
            layer.model = Model::Dielectric;
            layer.weight = dielectric * transmission;
            layer.alpha = alpha;
            layer.ior = ior;
            layer.thin = surface.thinWalled != 0U;
            layer.colour = specularColour;
            layer.transmitted = spectrum::reflectance(table, surface.transmissionColour, wavelengths);
            bsdf.add(layer);
        }
        const spectrum::SampledSpectrum opaque = dielectric * (1.0F - transmission);
        Closure specular;
        specular.model = Model::Specular;
        specular.weight = opaque;
        specular.alpha = alpha;
        specular.ior = relative(ior, coatIor, coat);
        specular.colour = specularColour;
        specular.film = film;
        specular.filmWeight = filmWeight;
        bsdf.add(specular);
        const float reflected = scattering::Specular::albedo(bsdf.albedos, cosine, std::sqrt(alpha.x * alpha.y), specular.ior);
        Closure lambertian;
        lambertian.model = Model::OrenNayar;
        lambertian.weight = opaque * filmed(specular, reflected, cosine, wavelengths);
        lambertian.colour = diffuse;
        lambertian.roughness = unit(surface.baseDiffuseRoughness);
        bsdf.add(lambertian);
    }

    /**
     * What a specular layer of an albedo lets through to the layer below it, at each wavelength.
     * That is one less its albedo, which a thin film scales by its reflectance over the bare
     * Fresnel factor at the cosine. The albedo is held to one.
     **/
    // NOLINTNEXTLINE(bugprone-easily-swappable-parameters) - the albedo, then the cosine
    MOYA_KERNEL static spectrum::SampledSpectrum filmed(const Closure & specular, float reflected, float cosine, const spectrum::SampledWavelengths & wavelengths) {
        const float bare = scattering::Fresnel::reflected(cosine, std::fmax(specular.ior, 1.0F));
        if (!specular.filmed() || !(bare > 0.0F)) {
            return spectrum::SampledSpectrum(1.0F - reflected);
        }
        const spectrum::SampledSpectrum factor = scattering::Specular::fresnel(specular, cosine, wavelengths);
        spectrum::SampledSpectrum out;
        for (int i = 0; i < spectrum::kSamples; ++i) {
            out[i] = 1.0F - std::fmin(reflected * factor[i] / bare, 1.0F);
        }
        return out;
    }

    /**
     * The index whose reflectance at normal incidence is an index's scaled by a weight, as
     * OpenPBR's specular weight scales it, at most kMostNormalReflectance.
     **/
    // NOLINTNEXTLINE(bugprone-easily-swappable-parameters) - the index, then the weight
    MOYA_KERNEL static float modulated(float ior, float weight) {
        const float root = (ior - 1.0F) / (ior + 1.0F);
        const float scaled = std::sqrt(std::fmin(std::fmax(weight * root * root, 0.0F), kMostNormalReflectance));
        return (1.0F + scaled) / (1.0F - scaled);
    }

    /**
     * The index of the specular layer relative to the medium over it: over the coat's index mixed
     * in by its weight. Below one, it is taken as its reciprocal, as the interface reflects alike.
     **/
    // NOLINTNEXTLINE(bugprone-easily-swappable-parameters) - the index, the coat's, then its weight
    MOYA_KERNEL static float relative(float ior, float coatIor, float coat) {
        const float out = ior / (1.0F + ((coatIor - 1.0F) * coat));
        return out < 1.0F ? 1.0F / out : out;
    }

    /**
     * How much the coat darkens the base below it, as OpenPBR gives it: one less K, over one less
     * the base's albedo times K, mixed in by the coat's darkening. K is the share of light the coat
     * reflects back down. For a smooth base it is Fresnel's factor inside the coat at the angle
     * light refracts to. For a rough base it is one less the mean a smooth interface transmits,
     * over the index squared. The two are mixed by the base's roughness.
     **/
    // NOLINTNEXTLINE(bugprone-easily-swappable-parameters) - the index, roughness, cosine, amount
    MOYA_KERNEL static spectrum::SampledSpectrum darkening(const Albedos & albedos, const spectrum::SampledSpectrum & albedo, float coatIor, float roughness,
        float cosine, float amount) {
        const float squared = (1.0F - (cosine * cosine)) / (coatIor * coatIor);
        const float refracted = std::sqrt(std::fmax(0.0F, 1.0F - squared));
        const float smooth = scattering::Fresnel::reflected(refracted, 1.0F / coatIor);
        const float rough = 1.0F - (albedos.transmittedAt(coatIor) / (coatIor * coatIor));
        const float back = smooth + ((rough - smooth) * roughness);
        spectrum::SampledSpectrum out;
        for (int i = 0; i < spectrum::kSamples; ++i) {
            const float denominator = 1.0F - (albedo[i] * back);
            const float darker = denominator > kLeastDarkening ? (1.0F - back) / denominator : 1.0F;
            out[i] = 1.0F + ((darker - 1.0F) * amount);
        }
        return out;
    }

 private:
    /** A value held to [0, 1]. **/
    MOYA_KERNEL static float unit(float value) {
        return std::fmin(std::fmax(value, 0.0F), 1.0F);
    }

    /** One value mixed into another by a share. **/
    template <typename T>
    MOYA_KERNEL static T mix(const T & from, const T & to, float share) {
        return from + ((to - from) * share);
    }
};

};  // namespace v3d::moya::kernel::bsdf
