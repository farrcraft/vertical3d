/*
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 */

#pragma once

#include <moya/libmoya/kernel/bsdf/Albedos.h>
#include <moya/libmoya/kernel/bsdf/Closure.h>
#include <moya/libmoya/kernel/bsdf/scattering/Diffuse.h>
#include <moya/libmoya/kernel/bsdf/scattering/Fresnel.h>
#include <moya/libmoya/kernel/bsdf/scattering/Ggx.h>
#include <moya/libmoya/kernel/bsdf/Singular.h>
#include <moya/libmoya/kernel/Qualifiers.h>
#include <moya/libmoya/kernel/spectrum/Spectrum.h>

#include <cmath>

#include <glm/geometric.hpp>
#include <glm/vec2.hpp>
#include <glm/vec3.hpp>

namespace v3d::moya::kernel::bsdf::scattering {

/** The least share of energy a single scattering loses on each side before it is given back. **/
inline constexpr float kLeastLoss = 1.0e-5F;

/**
 * The interface between two dielectrics, in a frame whose z is the shading normal and whose x is
 * the tangent. The shading normal points outside, where the index is one, and the closure's index
 * is inside's. An index of one or below lets light straight through. It reflects light on the side
 * it leaves towards and transmits it to the other side.
 *
 * - **A single scattering** is GGX's, by the closure's alphas. Each microfacet reflects by
 *   Fresnel's equations at the angle between outgoing and its normal, and refracts the rest.
 * - **The closure's colour tints its reflection**, and its transmitted colour its transmission.
 * - **A transmission's value is the radiance it carries**, divided by the square of the ratio of
 *   the indices, eta, the index of incoming's side over outgoing's. Its value over the square of
 *   the index on outgoing's side is reciprocal.
 * - **What a single scattering loses** is given back by Kulla and Conty's compensation. It is a
 *   lobe of one minus outgoing's albedo, times one minus incoming's albedo on incoming's side, over
 *   pi times one minus that side's mean albedo. A share of it is transmitted, which energy and
 *   reciprocity across the interface fix together: from outside, the share a smooth interface
 *   transmits, and from inside, whatever keeps the two reciprocal. When that would exceed one,
 *   inside's share is one and outside's follows from it.
 * - **A thin slab** has the same medium on both sides. It reflects as a GGX reflection by the
 *   slab's Fresnel factor, counting the light that bounces inside it, and transmits the rest along
 *   the reflection's mirror image through the surface. Its compensation is a reflection's, the
 *   slab's mean reflectance of it reflected and the rest transmitted.
 * - **A sample** draws a visible normal with the chance of outgoing's albedo, then reflects about
 *   it by its Fresnel factor or refracts through it. Otherwise it draws by the cosine on the side
 *   the compensation's share picks.
 *
 * The albedos are read at the geometric mean of the two alphas.
 **/
struct Dielectric {
    /** The value for two directions, before the closure's weight. **/
    MOYA_KERNEL static spectrum::SampledSpectrum evaluate(const Closure & closure, const Albedos & albedos, glm::vec3 outgoing, glm::vec3 incoming) {
        const bool inside = upward(outgoing, incoming);
        if (!(outgoing.z > 0.0F) || incoming.z == 0.0F) {
            return spectrum::SampledSpectrum(0.0F);
        }
        const Ggx ggx { closure.alpha };
        const float alpha = shared(closure);
        if (closure.thin) {
            const bool crossed = incoming.z < 0.0F;
            const glm::vec3 mirrored { incoming.x, incoming.y, std::fabs(incoming.z) };
            const float reflected = Fresnel::slab(glm::dot(outgoing, glm::normalize(outgoing + mirrored)), index(closure));
            const float share = crossed ? 1.0F - albedos.slabAt(index(closure)) : albedos.slabAt(index(closure));
            const float value = ((crossed ? 1.0F - reflected : reflected) * ggx.reflected(outgoing, mirrored)) +
                (share * slabCompensation(albedos, alpha, outgoing.z, mirrored.z));
            return (crossed ? closure.transmitted : closure.colour) * value;
        }
        const float eta = inside ? 1.0F / index(closure) : index(closure);
        const Lost lost = lostOf(closure, albedos, outgoing.z, inside);
        if (incoming.z > 0.0F) {
            const glm::vec3 normal = glm::normalize(outgoing + incoming);
            const float compensation = lost.own * lost.reflected * (1.0F - albedos.dielectricAt(incoming.z, alpha, index(closure), inside));
            return closure.colour * ((Fresnel::reflected(glm::dot(outgoing, normal), eta) * ggx.reflected(outgoing, incoming)) + compensation);
        }
        const Refraction through = refraction(outgoing, incoming, eta);
        float single = 0.0F;
        if (through.found) {
            const float transmitted = 1.0F - Fresnel::reflected(through.outgoing, eta);
            single = transmitted * ggx.distribution(through.normal) * ggx.shadowing(outgoing, incoming) * std::fabs(through.incoming) * through.outgoing /
                (outgoing.z * -incoming.z * through.denominator * through.denominator);
        }
        const float compensation = lost.own * lost.crossed * (1.0F - albedos.dielectricAt(-incoming.z, alpha, index(closure), !inside));
        return closure.transmitted * ((single + compensation) / (eta * eta));
    }

    /** The density sample() draws incoming with, for outgoing. **/
    MOYA_KERNEL static float density(const Closure & closure, const Albedos & albedos, glm::vec3 outgoing, glm::vec3 incoming) {
        const bool inside = upward(outgoing, incoming);
        if (!(outgoing.z > 0.0F) || incoming.z == 0.0F) {
            return 0.0F;
        }
        const Ggx ggx { closure.alpha };
        const float cosine = std::fabs(incoming.z) * kInversePi;
        if (closure.thin) {
            const bool crossed = incoming.z < 0.0F;
            const glm::vec3 mirrored { incoming.x, incoming.y, std::fabs(incoming.z) };
            const float single = slabShare(albedos, shared(closure), outgoing.z);
            const float reflected = Fresnel::slab(glm::dot(outgoing, glm::normalize(outgoing + mirrored)), index(closure));
            const float side = crossed ? 1.0F - albedos.slabAt(index(closure)) : albedos.slabAt(index(closure));
            return (single * (crossed ? 1.0F - reflected : reflected) * ggx.reflectedDensity(outgoing, mirrored)) + ((1.0F - single) * side * cosine);
        }
        const float eta = inside ? 1.0F / index(closure) : index(closure);
        const Lost lost = lostOf(closure, albedos, outgoing.z, inside);
        const float single = 1.0F - lost.own;
        if (incoming.z > 0.0F) {
            const glm::vec3 normal = glm::normalize(outgoing + incoming);
            const float reflected = Fresnel::reflected(glm::dot(outgoing, normal), eta);
            return (single * reflected * ggx.reflectedDensity(outgoing, incoming)) + (lost.own * (1.0F - lost.crosses) * cosine);
        }
        const Refraction through = refraction(outgoing, incoming, eta);
        float drawn = 0.0F;
        if (through.found) {
            const float transmitted = 1.0F - Fresnel::reflected(through.outgoing, eta);
            drawn = single * transmitted * ggx.visible(outgoing, through.normal) * std::fabs(through.incoming) / (through.denominator * through.denominator);
        }
        return drawn + (lost.own * lost.crosses * cosine);
    }

    /**
     * A direction light comes in from, drawn from two random numbers and a third that picks a
     * side. A zero direction is no sample: a microfacet's reflection that falls below the surface,
     * or its refraction that comes out above it.
     **/
    MOYA_KERNEL static glm::vec3 sample(const Closure & closure, const Albedos & albedos, const glm::vec3 & outgoing, const glm::vec2 & u, float choice) {
        const bool inside = outgoing.z < 0.0F;
        const float side = inside ? -1.0F : 1.0F;
        const glm::vec3 up { outgoing.x, outgoing.y, outgoing.z * side };
        const Ggx ggx { closure.alpha };
        float single = 0.0F;
        float crosses = 0.0F;
        float eta = index(closure);
        if (closure.thin) {
            single = slabShare(albedos, shared(closure), up.z);
            crosses = 1.0F - albedos.slabAt(index(closure));
        } else {
            const Lost lost = lostOf(closure, albedos, up.z, inside);
            single = 1.0F - lost.own;
            crosses = lost.crosses;
            eta = inside ? 1.0F / index(closure) : index(closure);
        }
        glm::vec3 out { 0.0F };
        if (u.x < single) {
            const glm::vec3 normal = ggx.sampleVisible(up, glm::vec2 { u.x / single, u.y });
            const float facing = glm::dot(up, normal);
            const float reflected = closure.thin ? Fresnel::slab(facing, eta) : Fresnel::reflected(facing, eta);
            out = (2.0F * facing * normal) - up;
            const bool crossed = !(choice < reflected);
            if (crossed && !closure.thin) {
                if (!Fresnel::refract(up, normal, eta, out) || !(out.z < 0.0F)) {
                    return glm::vec3 { 0.0F };
                }
            } else if (!(out.z > 0.0F)) {
                return glm::vec3 { 0.0F };
            } else if (crossed) {
                out.z = -out.z;
            }
        } else {
            out = Diffuse::sample(up, glm::vec2 { (u.x - single) / (1.0F - single), u.y });
            if (!(choice < 1.0F - crosses)) {
                out.z = -out.z;
            }
        }
        return glm::vec3 { out.x, out.y, out.z * side };
    }

    /**
     * What a smooth dielectric scatters outgoing into: its mirror direction by its Fresnel factor,
     * or straight on through it, by the third random number. A thin slab passes light through
     * unbent, and an interface bends it by the ratio of the indices.
     **/
    MOYA_KERNEL static Singular scatter(const Closure & closure, const glm::vec3 & outgoing, float choice) {
        const bool inside = outgoing.z < 0.0F;
        const float cosine = std::fabs(outgoing.z);
        const float eta = closure.thin || !inside ? index(closure) : 1.0F / index(closure);
        const float reflected = closure.thin ? Fresnel::slab(cosine, eta) : Fresnel::reflected(cosine, eta);
        Singular out;
        if (choice < reflected) {
            out.direction = glm::vec3 { -outgoing.x, -outgoing.y, outgoing.z };
            out.value = closure.colour * reflected;
            out.chance = reflected;
            return out;
        }
        const float transmitted = 1.0F - reflected;
        out.chance = transmitted;
        if (closure.thin) {
            out.direction = -outgoing;
            out.value = closure.transmitted * transmitted;
            return out;
        }
        const float side = inside ? -1.0F : 1.0F;
        glm::vec3 bent { 0.0F };
        Fresnel::refract(glm::vec3 { outgoing.x, outgoing.y, cosine }, glm::vec3 { 0.0F, 0.0F, 1.0F }, eta, bent);
        out.direction = glm::vec3 { bent.x, bent.y, bent.z * side };
        out.value = closure.transmitted * (transmitted / (eta * eta));
        return out;
    }

 private:
    /**
     * What a single scattering loses at outgoing's cosine, and how the compensation gives it back:
     * the factors of its reflected and transmitted lobes, and the share of it transmitted.
     **/
    struct Lost {
        float own { 0.0F };
        float reflected { 0.0F };
        float crossed { 0.0F };
        float crosses { 0.0F };
    };

    /**
     * A refraction between two directions on either side. It holds the microfacet normal that
     * bends one into the other, facing outgoing, and each direction's cosine to it. The change of
     * variables between them divides by the square of their sum, the denominator. It is not found
     * when either direction faces the normal the wrong way.
     **/
    struct Refraction {
        glm::vec3 normal { 0.0F, 0.0F, 1.0F };
        float outgoing { 0.0F };
        float incoming { 0.0F };
        float denominator { 0.0F };
        bool found { false };
    };

    /** The closure's index of refraction, no less than one. **/
    MOYA_KERNEL static float index(const Closure & closure) {
        return std::fmax(closure.ior, 1.0F);
    }

    /** The alpha the tables are read at for a closure's two: their geometric mean. **/
    MOYA_KERNEL static float shared(const Closure & closure) {
        return std::sqrt(closure.alpha.x * closure.alpha.y);
    }

    /** Turns two directions to put outgoing above the surface. Whether it lay below, inside. **/
    MOYA_KERNEL static bool upward(glm::vec3 & outgoing, glm::vec3 & incoming) {
        if (outgoing.z < 0.0F) {
            outgoing.z = -outgoing.z;
            incoming.z = -incoming.z;
            return true;
        }
        return false;
    }

    /** The refraction between outgoing, above the surface, and incoming, below it. **/
    MOYA_KERNEL static Refraction refraction(const glm::vec3 & outgoing, const glm::vec3 & incoming, float eta) {
        Refraction out;
        const glm::vec3 sum = outgoing + (eta * incoming);
        if (!(glm::dot(sum, sum) > 0.0F)) {
            return out;
        }
        out.normal = glm::normalize(sum);
        if (out.normal.z < 0.0F) {
            out.normal = -out.normal;
        }
        out.outgoing = glm::dot(outgoing, out.normal);
        out.incoming = glm::dot(incoming, out.normal);
        out.denominator = out.incoming + (out.outgoing / eta);
        out.found = out.outgoing > 0.0F && out.incoming < 0.0F && out.denominator != 0.0F;
        return out;
    }

    /** What an interface's single scattering loses at a cosine on a side, and how it returns. **/
    // NOLINTNEXTLINE(bugprone-easily-swappable-parameters) - the cosine, then the side
    MOYA_KERNEL static Lost lostOf(const Closure & closure, const Albedos & albedos, float cosine, bool inside) {
        const float alpha = shared(closure);
        const float eta = index(closure);
        const float outside = 1.0F - albedos.dielectricAverageAt(alpha, eta, false);
        const float within = 1.0F - albedos.dielectricAverageAt(alpha, eta, true);
        Lost out;
        if (!(outside > kLeastLoss) || !(within > kLeastLoss)) {
            return out;
        }
        const float squared = eta * eta;
        float fromOutside = albedos.transmittedAt(eta);
        float fromInside = fromOutside * outside / (squared * within);
        if (fromInside > 1.0F) {
            fromInside = 1.0F;
            fromOutside = squared * within / outside;
        }
        out.crosses = inside ? fromInside : fromOutside;
        out.own = std::fmin(std::fmax(1.0F - albedos.dielectricAt(cosine, alpha, eta, inside), 0.0F), 1.0F);
        out.reflected = (1.0F - out.crosses) * kInversePi / (inside ? within : outside);
        out.crossed = out.crosses * kInversePi / (inside ? outside : within);
        return out;
    }

    /** A reflection's compensation for two cosines above the surface, white, before its share. **/
    // NOLINTNEXTLINE(bugprone-easily-swappable-parameters) - the alpha, then the two cosines
    MOYA_KERNEL static float slabCompensation(const Albedos & albedos, float alpha, float outgoing, float incoming) {
        const float mean = albedos.averageAt(alpha);
        if (!(mean < 1.0F)) {
            return 0.0F;
        }
        return (1.0F - albedos.at(outgoing, alpha)) * (1.0F - albedos.at(incoming, alpha)) * kInversePi / (1.0F - mean);
    }

    /** The chance a thin slab's sample draws a visible normal: outgoing's albedo, clamped. **/
    // NOLINTNEXTLINE(bugprone-easily-swappable-parameters) - the alpha, then the cosine
    MOYA_KERNEL static float slabShare(const Albedos & albedos, float alpha, float outgoing) {
        return std::fmin(std::fmax(albedos.at(outgoing, alpha), 0.0F), 1.0F);
    }
};

};  // namespace v3d::moya::kernel::bsdf::scattering
