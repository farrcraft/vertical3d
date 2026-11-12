/*
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 */

#pragma once

#include <moya/libmoya/kernel/bsdf/Albedos.h>
#include <moya/libmoya/kernel/bsdf/Closure.h>
#include <moya/libmoya/kernel/bsdf/Conductor.h>
#include <moya/libmoya/kernel/bsdf/Diffuse.h>
#include <moya/libmoya/kernel/bsdf/Mirror.h>
#include <moya/libmoya/kernel/bsdf/Model.h>
#include <moya/libmoya/kernel/bsdf/Sample.h>
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
 *   is drawn from again.
 * - **The density of a direction** sums each closure's density times the chance it is picked.
 * - **A singular closure** has no value and no density for any direction it is given. Its sample
 *   is specular, as Sample says.
 * - **The albedo table** is what a microfacet lobe's energy compensation reads.
 *
 * A reflection that would cross the geometric surface is refused: a direction on the other side
 * of it from outgoing has no value, no density and no sample, whatever the shading normal says.
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
     * the mean of a conductor's Fresnel factor. A closure of no albedo is left out. A BSDF that
     * holds kMostClosures replaces the one of the smallest albedo with it, while that is smaller.
     **/
    MOYA_KERNEL void add(Closure closure) {
        closure.albedo = closure.weight.average();
        if (closure.model == Model::Conductor) {
            closure.albedo *= closure.fresnel.average().average();
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
        if (!sameSide(outgoing, incoming)) {
            return spectrum::SampledSpectrum(0.0F);
        }
        return valueAt(frame.toLocal(outgoing), frame.toLocal(incoming));
    }

    /** The density sample() draws incoming with, for outgoing. **/
    MOYA_KERNEL float density(const glm::vec3 & outgoing, const glm::vec3 & incoming) const {
        if (!sameSide(outgoing, incoming)) {
            return 0.0F;
        }
        return densityAt(frame.toLocal(outgoing), frame.toLocal(incoming));
    }

    /** A direction light comes in from for outgoing, drawn from two random numbers. **/
    MOYA_KERNEL Sample sample(const glm::vec3 & outgoing, const glm::vec2 & u) const {
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
        out.lobe = closure.lobe();
        if (closure.singular()) {
            out.incoming = frame.toWorld(Mirror::sample(local));
            out.specular = true;
            if (sameSide(outgoing, out.incoming)) {
                out.value = closure.model == Model::Conductor ? closure.weight * Conductor::mirrored(closure, local) : closure.weight;
                out.density = chance;
            }
            return out;
        }
        const glm::vec2 stretched { std::fmin((u.x - below) / chance, kBelowOne), u.y };
        const glm::vec3 drawn = closure.model == Model::Conductor ? Conductor::sample(closure, albedos, local, stretched) : Diffuse::sample(local, stretched);
        out.incoming = frame.toWorld(drawn);
        if (sameSide(outgoing, out.incoming)) {
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
    /** Whether two directions lie on one side of the geometric surface. **/
    MOYA_KERNEL bool sameSide(const glm::vec3 & outgoing, const glm::vec3 & incoming) const {
        return glm::dot(outgoing, geometric) * glm::dot(incoming, geometric) > 0.0F;
    }

    /** The sum of the closures' values, for two directions in the shading frame. **/
    MOYA_KERNEL spectrum::SampledSpectrum valueAt(const glm::vec3 & outgoing, const glm::vec3 & incoming) const {
        spectrum::SampledSpectrum out(0.0F);
        for (std::uint32_t i = 0; i < count && i < kMostClosures; ++i) {
            const Closure & closure = closures[i];
            if (closure.singular()) {
                continue;
            }
            if (closure.model == Model::Conductor) {
                out += closure.weight * Conductor::evaluate(closure, albedos, outgoing, incoming);
            } else {
                out += closure.weight * Diffuse::evaluate(outgoing, incoming);
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
            const float own = closure.model == Model::Conductor ? Conductor::density(closure, albedos, outgoing, incoming) : Diffuse::density(outgoing, incoming);
            out += (closure.albedo / total) * own;
        }
        return out;
    }
};

};  // namespace v3d::moya::kernel::bsdf
