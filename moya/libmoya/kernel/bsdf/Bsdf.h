/*
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 */

#pragma once

#include <moya/libmoya/kernel/bsdf/Albedos.h>
#include <moya/libmoya/kernel/bsdf/Closure.h>
#include <moya/libmoya/kernel/bsdf/scattering/Conductor.h>
#include <moya/libmoya/kernel/bsdf/scattering/Dielectric.h>
#include <moya/libmoya/kernel/bsdf/scattering/Fuzz.h>
#include <moya/libmoya/kernel/bsdf/scattering/Diffuse.h>
#include <moya/libmoya/kernel/bsdf/scattering/Mirror.h>
#include <moya/libmoya/kernel/bsdf/Model.h>
#include <moya/libmoya/kernel/bsdf/scattering/OrenNayar.h>
#include <moya/libmoya/kernel/bsdf/Sample.h>
#include <moya/libmoya/kernel/bsdf/Singular.h>
#include <moya/libmoya/kernel/bsdf/scattering/Specular.h>
#include <moya/libmoya/kernel/Qualifiers.h>
#include <moya/libmoya/kernel/spectrum/Spectrum.h>
#include <moya/libmoya/kernel/spectrum/Wavelengths.h>
#include <moya/libmoya/kernel/surface/Frame.h>

#include <cmath>
#include <cstdint>

#include <glm/geometric.hpp>
#include <glm/vec2.hpp>
#include <glm/vec3.hpp>

namespace v3d::moya::kernel::bsdf {

/** The most closures a BSDF holds. **/
inline constexpr std::uint32_t kMostClosures = 8;

/** The largest float below one. **/
inline constexpr float kBelowOne = 0x1.fffffep-1F;

/**
 * How a shading point scatters light: a list of weighted closures, each one lobe. Directions are
 * in world space and point away from the surface: outgoing towards where the light goes, incoming
 * towards where it comes from. Each closure works in a frame around a normal, whose tangent a
 * microfacet lobe's anisotropy follows: a coated closure in the coat's frame, and every other in
 * the shading frame.
 *
 * - **Its value is the sum of its closures' values**, each times its weight.
 * - **Its projected value** is the sum of each closure's value times the cosine of incoming to its
 *   own frame's normal. That is what light arriving along incoming is multiplied by.
 * - **A sample picks one closure** by its estimated albedo and draws a direction from it. The
 *   first random number picks the closure and is then stretched over the closure's share, so it
 *   is drawn from again. A third number picks between a dielectric's reflection and transmission.
 * - **The density of a direction** sums each closure's density times the chance it is picked.
 * - **A singular closure** has no value and no density for any direction it is given. Its sample
 *   is specular, as Sample says.
 * - **The albedo tables** are what a microfacet lobe's energy compensation reads, and the
 *   wavelengths what a thin film's interference reads.
 *
 * The geometric surface and a closure's frame have to agree on whether two directions lie on one
 * side or cross the surface. Where they disagree, the closure gives the pair no value, no density
 * and no sample, so a reflection never crosses the geometric surface and a transmission always
 * does.
 **/
struct Bsdf {
    Closure closures[kMostClosures];
    std::uint32_t count { 0 };
    surface::Frame frame;
    surface::Frame coatFrame;
    glm::vec3 geometric { 0.0F, 0.0F, 1.0F };
    Albedos albedos;
    spectrum::SampledWavelengths wavelengths;

    /** Adds a closure of a model and a weight, as add() adds any closure. **/
    MOYA_KERNEL void add(Model model, const spectrum::SampledSpectrum & weight) {
        Closure closure;
        closure.model = model;
        closure.weight = weight;
        add(closure);
    }

    /**
     * Adds a closure, estimating its albedo as the mean of its weight over the wavelengths, times
     * what its model reflects. That is a conductor's Fresnel factor's mean, or an Oren-Nayar lobe's
     * colour's. A specular reflection's is its colour's mean times the mean a smooth interface
     * reflects, and a dielectric's the larger mean of its two colours. Fuzz's is its colour's mean
     * times its albedo at a cosine of one half. A closure of no albedo is
     * left out. A BSDF that holds kMostClosures replaces the one of the smallest albedo with it,
     * while that is smaller. The albedo tables are set before a specular reflection is added.
     **/
    MOYA_KERNEL void add(Closure closure) {
        closure.albedo = closure.weight.average();
        switch (closure.model) {
            case Model::Conductor:
                closure.albedo *= closure.fresnel.average().average();
                break;
            case Model::OrenNayar:
                closure.albedo *= closure.colour.average();
                break;
            case Model::Specular:
                closure.albedo *= closure.colour.average() * (1.0F - albedos.transmittedAt(std::fmax(closure.ior, 1.0F)));
                break;
            case Model::Dielectric:
                closure.albedo *= std::fmax(closure.colour.average(), closure.transmitted.average());
                break;
            case Model::Fuzz:
                closure.albedo *= closure.colour.average() * scattering::Fuzz::albedo(0.5F, closure.roughness);
                break;
            case Model::Diffuse:
            case Model::Mirror:
                break;
        }
        if (!(closure.albedo > 0.0F)) {
            return;
        }
        std::uint32_t place = count;
        if (count >= kMostClosures) {
            place = 0;
            for (std::uint32_t i = 1; i < kMostClosures; ++i) {
                if (closures[i].albedo < closures[place].albedo) {
                    place = i;
                }
            }
            if (!(closures[place].albedo < closure.albedo)) {
                return;
            }
        } else {
            ++count;
        }
        closures[place] = closure;
    }

    /** The frame a closure works in: the coat's for the coat, and the shading frame otherwise. **/
    MOYA_KERNEL const surface::Frame & frameOf(const Closure & closure) const {
        return closure.coated ? coatFrame : frame;
    }

    /** The value for light coming in along incoming and leaving along outgoing. **/
    MOYA_KERNEL spectrum::SampledSpectrum evaluate(const glm::vec3 & outgoing, const glm::vec3 & incoming) const {
        return summed(outgoing, incoming, false);
    }

    /** The value with each closure's times its cosine of incoming to its frame's normal. **/
    MOYA_KERNEL spectrum::SampledSpectrum projected(const glm::vec3 & outgoing, const glm::vec3 & incoming) const {
        return summed(outgoing, incoming, true);
    }

    /** The density sample() draws incoming with, for outgoing. **/
    MOYA_KERNEL float density(const glm::vec3 & outgoing, const glm::vec3 & incoming) const {
        const float total = albedo();
        if (!(total > 0.0F)) {
            return 0.0F;
        }
        const Directions shaded = directionsIn(frame, outgoing, incoming);
        const Directions coated = directionsIn(coatFrame, outgoing, incoming);
        float out = 0.0F;
        for (std::uint32_t i = 0; i < count && i < kMostClosures; ++i) {
            const Closure & closure = closures[i];
            const Directions & local = closure.coated ? coated : shaded;
            if (closure.singular() || !local.agree) {
                continue;
            }
            out += (closure.albedo / total) * densityOf(closure, local.outgoing, local.incoming);
        }
        return out;
    }

    /**
     * A direction light comes in from for outgoing, drawn from two random numbers and a third. Its
     * value, projected value and density are the whole BSDF's, over every closure.
     **/
    MOYA_KERNEL Sample sample(const glm::vec3 & outgoing, const glm::vec2 & u, float choice) const {
        Sample out;
        const float total = albedo();
        if (!(total > 0.0F)) {
            return out;
        }
        std::uint32_t picked = 0;
        float below = 0.0F;
        float chance = 0.0F;
        for (std::uint32_t i = 0; i < count && i < kMostClosures; ++i) {
            chance = closures[i].albedo / total;
            if (u.x < below + chance || i + 1U == count) {
                picked = i;
                break;
            }
            below += chance;
        }
        const Closure & closure = closures[picked];
        const surface::Frame & own = frameOf(closure);
        const glm::vec3 local = own.toLocal(outgoing);
        if (closure.singular()) {
            const Singular scattered = singular(closure, local, choice);
            out.incoming = own.toWorld(scattered.direction);
            out.specular = true;
            out.lobe = closure.lobe(scattered.direction.z * local.z < 0.0F);
            if (scattered.chance > 0.0F && agrees(outgoing, out.incoming, local, scattered.direction)) {
                out.value = closure.weight * scattered.value;
                out.projected = out.value;
                out.density = chance * scattered.chance;
            }
            return out;
        }
        const glm::vec2 stretched { std::fmin((u.x - below) / chance, kBelowOne), u.y };
        glm::vec3 drawn { 0.0F };
        switch (closure.model) {
            case Model::Conductor:
            case Model::Specular:
                drawn = scattering::Conductor::sample(closure, albedos, local, stretched);
                break;
            case Model::Dielectric:
                drawn = scattering::Dielectric::sample(closure, albedos, local, stretched, choice);
                break;
            case Model::Fuzz:
                drawn = scattering::Fuzz::sample(closure, local, stretched);
                break;
            case Model::Diffuse:
            case Model::Mirror:
            case Model::OrenNayar:
                drawn = scattering::Diffuse::sample(local, stretched);
                break;
        }
        if (drawn.x == 0.0F && drawn.y == 0.0F && drawn.z == 0.0F) {
            return out;
        }
        out.incoming = own.toWorld(drawn);
        out.lobe = closure.lobe(drawn.z * local.z < 0.0F);
        out.roughness = closure.spread();
        if (agrees(outgoing, out.incoming, local, drawn)) {
            out.value = evaluate(outgoing, out.incoming);
            out.projected = projected(outgoing, out.incoming);
            out.density = density(outgoing, out.incoming);
        }
        return out;
    }

    /** The sum of the closures' estimated albedos. **/
    MOYA_KERNEL float albedo() const {
        float out = 0.0F;
        for (std::uint32_t i = 0; i < count && i < kMostClosures; ++i) {
            out += closures[i].albedo;
        }
        return out;
    }

    /** The share of the closures' estimated albedos that its singular closures hold, or zero. **/
    MOYA_KERNEL float singularShare() const {
        float all = 0.0F;
        float singular = 0.0F;
        for (std::uint32_t i = 0; i < count && i < kMostClosures; ++i) {
            all += closures[i].albedo;
            singular += closures[i].singular() ? closures[i].albedo : 0.0F;
        }
        return all > 0.0F ? singular / all : 0.0F;
    }

    /**
     * The albedo of its closures that are not singular, at each wavelength: each one's weight
     * times what its model reflects, as add() estimates them but at each wavelength apart.
     **/
    MOYA_KERNEL spectrum::SampledSpectrum spreadAlbedo() const {
        spectrum::SampledSpectrum out(0.0F);
        for (std::uint32_t i = 0; i < count && i < kMostClosures; ++i) {
            if (!closures[i].singular()) {
                out += closures[i].weight * reflects(closures[i]);
            }
        }
        return out;
    }

 private:
    /** What a closure's model reflects at each wavelength, by the estimates add() averages. **/
    MOYA_KERNEL spectrum::SampledSpectrum reflects(const Closure & closure) const {
        switch (closure.model) {
            case Model::Conductor:
                return closure.fresnel.average();
            case Model::OrenNayar:
                return closure.colour;
            case Model::Specular:
                return closure.colour * (1.0F - albedos.transmittedAt(std::fmax(closure.ior, 1.0F)));
            case Model::Dielectric:
                return closure.colour.average() >= closure.transmitted.average() ? closure.colour : closure.transmitted;
            case Model::Fuzz:
                return closure.colour * scattering::Fuzz::albedo(0.5F, closure.roughness);
            case Model::Diffuse:
            case Model::Mirror:
                break;
        }
        return spectrum::SampledSpectrum(1.0F);
    }

    /** Two directions in a frame, and whether it and the geometric surface agree on them. **/
    struct Directions {
        glm::vec3 outgoing { 0.0F };
        glm::vec3 incoming { 0.0F };
        bool agree { false };
    };

    /** Two directions in world space, in a frame. **/
    MOYA_KERNEL Directions directionsIn(const surface::Frame & within, const glm::vec3 & outgoing, const glm::vec3 & incoming) const {
        Directions out;
        out.outgoing = within.toLocal(outgoing);
        out.incoming = within.toLocal(incoming);
        out.agree = agrees(outgoing, incoming, out.outgoing, out.incoming);
        return out;
    }

    /**
     * Whether the geometric surface and a frame agree that two directions lie on one side, or that
     * they lie on either side. The directions are given in world space and in the frame.
     **/
    // NOLINTNEXTLINE(bugprone-easily-swappable-parameters) - two in world space, two in the frame
    MOYA_KERNEL bool agrees(const glm::vec3 & outgoing, const glm::vec3 & incoming, const glm::vec3 & local, const glm::vec3 & towards) const {
        const float across = glm::dot(outgoing, geometric) * glm::dot(incoming, geometric);
        const float shaded = local.z * towards.z;
        return (across > 0.0F && shaded > 0.0F) || (across < 0.0F && shaded < 0.0F);
    }

    /** Where a singular closure scatters outgoing, in its frame, by the third number. **/
    MOYA_KERNEL Singular singular(const Closure & closure, const glm::vec3 & outgoing, float choice) const {
        Singular out;
        switch (closure.model) {
            case Model::Dielectric:
                return scattering::Dielectric::scatter(closure, outgoing, choice);
            case Model::Conductor:
                out.value = scattering::Conductor::mirrored(closure, wavelengths, outgoing);
                break;
            case Model::Specular:
                out.value = scattering::Specular::mirrored(closure, wavelengths, outgoing);
                break;
            case Model::Diffuse:
            case Model::Mirror:
            case Model::OrenNayar:
            case Model::Fuzz:
                break;
        }
        out.direction = scattering::Mirror::sample(outgoing);
        return out;
    }

    /**
     * The sum of the closures' values for two directions in world space, each closure's taken in
     * its own frame, and times its cosine of incoming where projecting.
     **/
    MOYA_KERNEL spectrum::SampledSpectrum summed(const glm::vec3 & outgoing, const glm::vec3 & incoming, bool projecting) const {
        const Directions shaded = directionsIn(frame, outgoing, incoming);
        const Directions coated = directionsIn(coatFrame, outgoing, incoming);
        spectrum::SampledSpectrum out(0.0F);
        for (std::uint32_t i = 0; i < count && i < kMostClosures; ++i) {
            const Closure & closure = closures[i];
            const Directions & local = closure.coated ? coated : shaded;
            if (closure.singular() || !local.agree) {
                continue;
            }
            const float cosine = projecting ? std::fabs(local.incoming.z) : 1.0F;
            out += closure.weight * valueOf(closure, local.outgoing, local.incoming) * cosine;
        }
        return out;
    }

    /** One closure's value, without its weight, for two directions in its frame. **/
    MOYA_KERNEL spectrum::SampledSpectrum valueOf(const Closure & closure, const glm::vec3 & outgoing, const glm::vec3 & incoming) const {
        switch (closure.model) {
            case Model::Conductor:
                return scattering::Conductor::evaluate(closure, albedos, wavelengths, outgoing, incoming);
            case Model::Dielectric:
                return scattering::Dielectric::evaluate(closure, albedos, outgoing, incoming);
            case Model::Specular:
                return scattering::Specular::evaluate(closure, albedos, wavelengths, outgoing, incoming);
            case Model::Fuzz:
                return scattering::Fuzz::evaluate(closure, outgoing, incoming);
            case Model::OrenNayar:
                return scattering::OrenNayar::evaluate(closure, outgoing, incoming);
            case Model::Diffuse:
            case Model::Mirror:
                break;
        }
        return spectrum::SampledSpectrum(scattering::Diffuse::evaluate(outgoing, incoming));
    }

    /** One closure's density, for two directions in its frame. **/
    MOYA_KERNEL float densityOf(const Closure & closure, const glm::vec3 & outgoing, const glm::vec3 & incoming) const {
        switch (closure.model) {
            case Model::Conductor:
            case Model::Specular:
                return scattering::Conductor::density(closure, albedos, outgoing, incoming);
            case Model::Dielectric:
                return scattering::Dielectric::density(closure, albedos, outgoing, incoming);
            case Model::Fuzz:
                return scattering::Fuzz::density(closure, outgoing, incoming);
            case Model::Diffuse:
            case Model::Mirror:
            case Model::OrenNayar:
                break;
        }
        return scattering::Diffuse::density(outgoing, incoming);
    }
};

};  // namespace v3d::moya::kernel::bsdf
