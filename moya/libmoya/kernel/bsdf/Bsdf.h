/*
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 */

#pragma once

#include <moya/libmoya/kernel/bsdf/Closure.h>
#include <moya/libmoya/kernel/bsdf/Diffuse.h>
#include <moya/libmoya/kernel/bsdf/Lobe.h>
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
 * towards where it comes from. Every closure works in the frame around the shading normal.
 *
 * - **Its value is the sum of its closures' values**, each times its weight.
 * - **A sample picks one closure** by its estimated albedo, the mean of its weight over the
 *   wavelengths, and draws a direction from it. The first random number picks the closure and is
 *   then stretched over the closure's share, so it is drawn from again.
 * - **The density of a direction** sums each closure's density times the chance it is picked.
 * - **A closure that scatters into a single direction** has no value and no density for any
 *   direction it is given. Its sample is specular, as Sample says.
 *
 * A reflection that would cross the geometric surface is refused: a direction on the other side
 * of it from outgoing has no value, no density and no sample, whatever the shading normal says.
 **/
struct Bsdf {
    Closure closures[kMostClosures];
    std::uint32_t count { 0 };
    surface::Frame frame;
    glm::vec3 geometric { 0.0F, 0.0F, 1.0F };

    /**
     * Adds a closure of a model and a weight. A closure of no albedo is left out. A BSDF that
     * holds kMostClosures replaces the one of the smallest albedo with it, while that is smaller.
     **/
    MOYA_KERNEL void add(Model model, const spectrum::SampledSpectrum & weight) {
        const float albedo = weight.average();
        if (!(albedo > 0.0F)) {
            return;
        }
        std::uint32_t place = count;
        if (count == kMostClosures) {
            place = 0;
            for (std::uint32_t i = 1; i < count; ++i) {
                if (closures[i].albedo < closures[place].albedo) {
                    place = i;
                }
            }
            if (!(closures[place].albedo < albedo)) {
                return;
            }
        } else {
            ++count;
        }
        closures[place] = Closure { .model = model, .weight = weight, .albedo = albedo };
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
        out.lobe = lobeOf(closure.model);
        if (singular(closure.model)) {
            out.incoming = frame.toWorld(Mirror::sample(local));
            out.specular = true;
            if (sameSide(outgoing, out.incoming)) {
                out.value = closure.weight;
                out.density = chance;
            }
            return out;
        }
        const glm::vec2 stretched { std::fmin((u.x - below) / chance, kBelowOne), u.y };
        const glm::vec3 drawn = Diffuse::sample(local, stretched);
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
        for (std::uint32_t i = 0; i < count; ++i) {
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
        for (std::uint32_t i = 0; i < count; ++i) {
            if (closures[i].model == Model::Diffuse) {
                out += closures[i].weight * Diffuse::evaluate(outgoing, incoming);
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
        for (std::uint32_t i = 0; i < count; ++i) {
            if (closures[i].model == Model::Diffuse) {
                out += (closures[i].albedo / total) * Diffuse::density(outgoing, incoming);
            }
        }
        return out;
    }
};

};  // namespace v3d::moya::kernel::bsdf
