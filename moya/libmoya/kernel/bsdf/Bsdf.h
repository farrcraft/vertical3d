/*
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 */

#pragma once

#include <moya/libmoya/kernel/bsdf/Albedos.h>
#include <moya/libmoya/kernel/bsdf/Closure.h>
#include <moya/libmoya/kernel/bsdf/scattering/Conductor.h>
#include <moya/libmoya/kernel/bsdf/scattering/Dielectric.h>
#include <moya/libmoya/kernel/bsdf/scattering/Diffuse.h>
#include <moya/libmoya/kernel/bsdf/scattering/Mirror.h>
#include <moya/libmoya/kernel/bsdf/Model.h>
#include <moya/libmoya/kernel/bsdf/scattering/OrenNayar.h>
#include <moya/libmoya/kernel/bsdf/Sample.h>
#include <moya/libmoya/kernel/bsdf/Singular.h>
#include <moya/libmoya/kernel/bsdf/scattering/Specular.h>
#include <moya/libmoya/kernel/Qualifiers.h>
#include <moya/libmoya/kernel/spectrum/Spectrum.h>
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
 * towards where it comes from. Every closure works in the frame around the shading normal, whose
 * tangent a microfacet lobe's anisotropy follows.
 *
 * - **Its value is the sum of its closures' values**, each times its weight.
 * - **A sample picks one closure** by its estimated albedo and draws a direction from it. The
 *   first random number picks the closure and is then stretched over the closure's share, so it
 *   is drawn from again. A third number picks between a dielectric's reflection and transmission.
 * - **The density of a direction** sums each closure's density times the chance it is picked.
 * - **A singular closure** has no value and no density for any direction it is given. Its sample
 *   is specular, as Sample says.
 * - **The albedo tables** are what a microfacet lobe's energy compensation reads.
 *
 * The geometric surface and the shading normal have to agree on whether two directions lie on one
 * side or cross the surface. Where they disagree, the pair has no value, no density and no
 * sample, so a reflection never crosses the geometric surface and a transmission always does.
 **/
struct Bsdf {
    Closure closures[kMostClosures];
    std::uint32_t count { 0 };
    surface::Frame frame;
    glm::vec3 geometric { 0.0F, 0.0F, 1.0F };
    Albedos albedos;

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
     * reflects, and a dielectric's the larger mean of its two colours. A closure of no albedo is
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

    /** The value for light coming in along incoming and leaving along outgoing. **/
    MOYA_KERNEL spectrum::SampledSpectrum evaluate(const glm::vec3 & outgoing, const glm::vec3 & incoming) const {
        const glm::vec3 local = frame.toLocal(outgoing);
        const glm::vec3 towards = frame.toLocal(incoming);
        if (!agrees(outgoing, incoming, local, towards)) {
            return spectrum::SampledSpectrum(0.0F);
        }
        return valueAt(local, towards);
    }

    /** The density sample() draws incoming with, for outgoing. **/
    MOYA_KERNEL float density(const glm::vec3 & outgoing, const glm::vec3 & incoming) const {
        const glm::vec3 local = frame.toLocal(outgoing);
        const glm::vec3 towards = frame.toLocal(incoming);
        if (!agrees(outgoing, incoming, local, towards)) {
            return 0.0F;
        }
        return densityAt(local, towards);
    }

    /** A direction light comes in from for outgoing, drawn from two random numbers and a third. **/
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
        const glm::vec3 local = frame.toLocal(outgoing);
        if (closure.singular()) {
            const Singular scattered = singular(closure, local, choice);
            out.incoming = frame.toWorld(scattered.direction);
            out.specular = true;
            out.lobe = closure.lobe(scattered.direction.z * local.z < 0.0F);
            if (scattered.chance > 0.0F && agrees(outgoing, out.incoming, local, scattered.direction)) {
                out.value = closure.weight * scattered.value;
                out.density = chance * scattered.chance;
            }
            return out;
        }
        const glm::vec2 stretched { std::fmin((u.x - below) / chance, kBelowOne), u.y };
        glm::vec3 drawn { 0.0F };
        switch (closure.model) {
            case Model::Conductor:
                drawn = scattering::Conductor::sample(closure, albedos, local, stretched);
                break;
            case Model::Specular:
                drawn = scattering::Conductor::sample(closure, albedos, local, stretched);
                break;
            case Model::Dielectric:
                drawn = scattering::Dielectric::sample(closure, albedos, local, stretched, choice);
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
        out.incoming = frame.toWorld(drawn);
        out.lobe = closure.lobe(drawn.z * local.z < 0.0F);
        if (agrees(outgoing, out.incoming, local, drawn)) {
            out.value = valueAt(local, drawn);
            out.density = densityAt(local, drawn);
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

 private:
    /**
     * Whether the geometric surface and the shading frame agree that two directions lie on one
     * side, or that they lie on either side. The directions are given in world space and in the
     * shading frame.
     **/
    MOYA_KERNEL bool agrees(const glm::vec3 & outgoing, const glm::vec3 & incoming, const glm::vec3 & local, const glm::vec3 & towards) const {
        const float across = glm::dot(outgoing, geometric) * glm::dot(incoming, geometric);
        const float shaded = local.z * towards.z;
        return (across > 0.0F && shaded > 0.0F) || (across < 0.0F && shaded < 0.0F);
    }

    /** Where a singular closure scatters outgoing, in the shading frame, by the third number. **/
    MOYA_KERNEL static Singular singular(const Closure & closure, const glm::vec3 & outgoing, float choice) {
        Singular out;
        switch (closure.model) {
            case Model::Dielectric:
                return scattering::Dielectric::scatter(closure, outgoing, choice);
            case Model::Conductor:
                out.value = scattering::Conductor::mirrored(closure, outgoing);
                break;
            case Model::Specular:
                out.value = scattering::Specular::mirrored(closure, outgoing);
                break;
            case Model::Diffuse:
            case Model::Mirror:
            case Model::OrenNayar:
                break;
        }
        out.direction = scattering::Mirror::sample(outgoing);
        return out;
    }

    /** The sum of the closures' values, for two directions in the shading frame. **/
    MOYA_KERNEL spectrum::SampledSpectrum valueAt(const glm::vec3 & outgoing, const glm::vec3 & incoming) const {
        spectrum::SampledSpectrum out(0.0F);
        for (std::uint32_t i = 0; i < count && i < kMostClosures; ++i) {
            const Closure & closure = closures[i];
            if (closure.singular()) {
                continue;
            }
            switch (closure.model) {
                case Model::Conductor:
                    out += closure.weight * scattering::Conductor::evaluate(closure, albedos, outgoing, incoming);
                    break;
                case Model::Dielectric:
                    out += closure.weight * scattering::Dielectric::evaluate(closure, albedos, outgoing, incoming);
                    break;
                case Model::Specular:
                    out += closure.weight * scattering::Specular::evaluate(closure, albedos, outgoing, incoming);
                    break;
                case Model::OrenNayar:
                    out += closure.weight * scattering::OrenNayar::evaluate(closure, outgoing, incoming);
                    break;
                case Model::Diffuse:
                case Model::Mirror:
                    out += closure.weight * scattering::Diffuse::evaluate(outgoing, incoming);
                    break;
            }
        }
        return out;
    }

    /** The density sample() draws incoming with, for two directions in the shading frame. **/
    MOYA_KERNEL float densityAt(const glm::vec3 & outgoing, const glm::vec3 & incoming) const {
        const float total = albedo();
        if (!(total > 0.0F)) {
            return 0.0F;
        }
        float out = 0.0F;
        for (std::uint32_t i = 0; i < count && i < kMostClosures; ++i) {
            const Closure & closure = closures[i];
            if (closure.singular()) {
                continue;
            }
            float own = 0.0F;
            switch (closure.model) {
                case Model::Conductor:
                case Model::Specular:
                    own = scattering::Conductor::density(closure, albedos, outgoing, incoming);
                    break;
                case Model::Dielectric:
                    own = scattering::Dielectric::density(closure, albedos, outgoing, incoming);
                    break;
                case Model::Diffuse:
                case Model::Mirror:
                case Model::OrenNayar:
                    own = scattering::Diffuse::density(outgoing, incoming);
                    break;
            }
            out += (closure.albedo / total) * own;
        }
        return out;
    }
};

};  // namespace v3d::moya::kernel::bsdf
