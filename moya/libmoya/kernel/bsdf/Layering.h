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

#include <glm/common.hpp>
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
     * leaving at a cosine to the shading frame's normal and at another to the coat frame's.
     * Colours become spectra through the table. The coat's albedo and the darkening it gives
     * are taken at the coat's cosine, and every other layer's at the shading frame's.
     *
     * - **The fuzz**, of its weight, tinted by its colour, lies over everything. What lies below
     *   takes one less its weight times its albedo, untinted.
     * - **The coat**, of its weight, is a specular reflection of its own roughness and index, in
     *   the coat's frame. What
     *   lies below takes one less its weight, and its weight times one less its albedo, its colour
     *   and its darkening.
     * - **The coat roughens the layers below it.** Their roughness becomes the fourth root of its
     *   fourth power plus twice the coat's, at most one, mixed in by the coat's weight.
     * - **The metal**, of the base's metalness, is a conductor of the base's colour and the
     *   specular colour as its F82 tint, scaled by the specular weight.
     * - **The rest is a dielectric.** Its index is the specular index with its reflectance at
     *   normal incidence scaled by the specular weight. Of the transmission's weight, it is a
     *   dielectric that transmits, tinted by the specular colour and the transmission colour. A
     *   surface that is not thin-walled and has a transmission depth is not tinted by the
     *   transmission colour, which the medium inside takes over that depth. Its index is relative
     *   to that of the medium outside, which the BSDF holds.
     * - **The opaque rest** is a specular reflection over the diffuse base, its index relative to
     *   the coat's where the coat lies over it. The diffuse base is an Oren-Nayar lobe of the
     *   base's roughness, its colour the base colour times its weight. It takes one less the
     *   specular layer's albedo, and one less the subsurface weight.
     * - **The subsurface**, of its weight, takes the rest of the diffuse base's share.
     * - **On a surface that is not thin-walled**, the subsurface takes the same share of the
     *   opaque rest's specular reflection too. It is a dielectric that walks, of the specular
     *   layer's roughness and index, its reflection tinted by the specular colour. A path it
     *   carries into the surface walks the medium the subsurface gives. A denoiser sees it as the
     *   subsurface colour.
     * - **On a thin wall**, the subsurface is two Oren-Nayar lobes of the base's roughness,
     *   coloured by the subsurface colour. One reflects, of half of one less the subsurface's
     *   anisotropy. The other transmits, of half of one plus it.
     * - **The coat's darkening** takes the base's albedo as the base colour mixed with the
     *   subsurface colour by the subsurface weight.
     * - **A thin film**, of its weight, lies over the metal and the opaque specular layer, its
     *   index relative to the coat's as the specular layer's is. The layer below the specular
     *   layer takes one less its albedo times its film's reflectance over its bare Fresnel factor,
     *   at outgoing's cosine and each wavelength.
     **/
    // NOLINTNEXTLINE(bugprone-easily-swappable-parameters) - the frame's cosine, then the coat's
    MOYA_KERNEL static void add(Bsdf & bsdf, const surface::OpenPbr & surface, const spectrum::RgbTable & table, const spectrum::SampledWavelengths & wavelengths,
        float cosine, float coatCosine) {
        const float coat = unit(surface.coatWeight);
        const float metal = unit(surface.baseMetalness);
        const float transmission = unit(surface.transmissionWeight);
        const float coatIor = std::fmax(surface.coatIor, 1.0F);
        const glm::vec3 base = surface.baseWeight * surface.baseColour;
        const float subsurface = unit(surface.subsurfaceWeight);
        const bool thin = surface.thinWalled != 0U;
        const float walked = thin ? 0.0F : subsurface;
        const spectrum::SampledSpectrum diffuse = spectrum::reflectance(table, base, wavelengths);
        const spectrum::SampledSpectrum estimated = spectrum::reflectance(table, mix(base, surface.subsurfaceColour, subsurface), wavelengths);
        const spectrum::SampledSpectrum & metallic = diffuse;
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
            layer.coated = true;
            bsdf.add(layer);
            const float coatRoughness = unit(surface.coatRoughness);
            const float roughened = std::fmin(1.0F, std::pow(std::pow(roughness, 4.0F) + (2.0F * std::pow(coatRoughness, 4.0F)), 0.25F));
            roughness += (roughened - roughness) * coat;
            const float reflected = scattering::Specular::albedo(bsdf.albedos, coatCosine, std::sqrt(layer.alpha.x * layer.alpha.y), coatIor);
            const spectrum::SampledSpectrum lowest = mix(estimated * (1.0F - transmission), metallic, metal);
            const float baseRoughness = roughness + ((1.0F - roughness) * (1.0F - metal) * (1.0F - transmission));
            const spectrum::SampledSpectrum darkened = darkening(bsdf.albedos, lowest, coatIor, baseRoughness, coatCosine, unit(surface.coatDarkening));
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
            layer.ior = ior / bsdf.outside;
            layer.thin = surface.thinWalled != 0U;
            layer.colour = specularColour;
            const bool deep = surface.thinWalled == 0U && surface.transmissionDepth > 0.0F;
            layer.transmitted = deep ? spectrum::SampledSpectrum(1.0F) : spectrum::reflectance(table, surface.transmissionColour, wavelengths);
            bsdf.add(layer);
        }
        const spectrum::SampledSpectrum opaque = dielectric * (1.0F - transmission);
        if (walked > 0.0F) {
            Closure walk = walking(opaque * walked, alpha, ior / bsdf.outside, specularColour);
            walk.seen = spectrum::reflectance(table, glm::max(surface.subsurfaceColour, glm::vec3(0.0F)), wavelengths);
            bsdf.add(walk);
        }
        Closure specular;
        specular.model = Model::Specular;
        specular.weight = opaque * (1.0F - walked);
        specular.alpha = alpha;
        specular.ior = relative(ior, coatIor, coat);
        specular.colour = specularColour;
        specular.film = film;
        specular.filmWeight = filmWeight;
        bsdf.add(specular);
        const float reflected = scattering::Specular::albedo(bsdf.albedos, cosine, std::sqrt(alpha.x * alpha.y), specular.ior);
        const spectrum::SampledSpectrum under = specular.weight * filmed(specular, reflected, cosine, wavelengths);
        Closure lambertian;
        lambertian.model = Model::OrenNayar;
        // a walk took its share from the specular reflection, and a thin wall's sheet takes it here
        lambertian.weight = under * (1.0F - (subsurface - walked));
        lambertian.colour = diffuse;
        lambertian.roughness = unit(surface.baseDiffuseRoughness);
        bsdf.add(lambertian);
        if (thin && subsurface > 0.0F) {
            sheet(bsdf, lambertian, under * subsurface, surface.subsurfaceScatterAnisotropy,
                spectrum::reflectance(table, glm::max(surface.subsurfaceColour, glm::vec3(0.0F)), wavelengths));
        }
    }

    /**
     * The closure a path walking an OpenPBR surface's subsurface meets the surface by from inside:
     * the subsurface's dielectric alone, of weight one. Its index is relative to the medium beyond
     * the surface, which the BSDF holds.
     **/
    MOYA_KERNEL static void walk(Bsdf & bsdf, const surface::OpenPbr & surface, const spectrum::RgbTable & table, const spectrum::SampledWavelengths & wavelengths) {
        bsdf.wavelengths = wavelengths;
        const glm::vec2 alpha = scattering::Ggx::alphas(unit(surface.specularRoughness), unit(surface.specularAnisotropy));
        const float ior = modulated(std::fmax(surface.specularIor, 1.0F), std::fmax(surface.specularWeight, 0.0F));
        bsdf.add(walking(spectrum::SampledSpectrum(1.0F), alpha, ior / bsdf.outside, spectrum::reflectance(table, surface.specularColour, wavelengths)));
    }

    /** OpenPBR's closures, for light leaving at one cosine to both frames' normals. **/
    MOYA_KERNEL static void add(Bsdf & bsdf, const surface::OpenPbr & surface, const spectrum::RgbTable & table, const spectrum::SampledWavelengths & wavelengths,
        float cosine) {
        add(bsdf, surface, table, wavelengths, cosine, cosine);
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
    // NOLINTNEXTLINE(bugprone-easily-swappable-parameters) - the index, roughness, cosine, share
    MOYA_KERNEL static spectrum::SampledSpectrum darkening(const Albedos & albedos, const spectrum::SampledSpectrum & base, float ior, float roughness, float cosine, float share) {
        const float squared = (1.0F - (cosine * cosine)) / (ior * ior);
        const float refracted = std::sqrt(std::fmax(0.0F, 1.0F - squared));
        const float smooth = scattering::Fresnel::reflected(refracted, 1.0F / ior);
        const float rough = 1.0F - (albedos.transmittedAt(ior) / (ior * ior));
        const float back = smooth + ((rough - smooth) * roughness);
        spectrum::SampledSpectrum out;
        for (int i = 0; i < spectrum::kSamples; ++i) {
            const float denominator = 1.0F - (base[i] * back);
            const float darker = denominator > kLeastDarkening ? (1.0F - back) / denominator : 1.0F;
            out[i] = 1.0F + ((darker - 1.0F) * share);
        }
        return out;
    }

 private:
    /** The subsurface's dielectric of a weight, alphas, an index and a reflection's colour. **/
    // NOLINTNEXTLINE(bugprone-easily-swappable-parameters) - the alphas, then the index
    MOYA_KERNEL static Closure walking(const spectrum::SampledSpectrum & weight, const glm::vec2 & alpha, float ior, const spectrum::SampledSpectrum & colour) {
        Closure out;
        out.model = Model::Dielectric;
        out.weight = weight;
        out.alpha = alpha;
        out.ior = ior;
        out.colour = colour;
        out.walks = true;
        return out;
    }

    /**
     * Adds a thin wall's subsurface sheet of a weight, as two lobes like a diffuse base of a
     * colour. One reflects half the weight times one less the anisotropy. The other goes through
     * the surface, of half the weight times one plus it.
     **/
    // NOLINTNEXTLINE(bugprone-easily-swappable-parameters) - the weight, then the colour
    MOYA_KERNEL static void sheet(Bsdf & bsdf, const Closure & base, const spectrum::SampledSpectrum & weight, float anisotropy,
        const spectrum::SampledSpectrum & colour) {
        const float forwards = std::fmin(std::fmax(anisotropy, -1.0F), 1.0F);
        Closure out = base;
        out.colour = colour;
        out.weight = weight * (0.5F * (1.0F - forwards));
        bsdf.add(out);
        out.weight = weight * (0.5F * (1.0F + forwards));
        out.through = true;
        bsdf.add(out);
    }

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
