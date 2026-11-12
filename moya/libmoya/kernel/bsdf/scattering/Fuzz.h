/*
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 */

#pragma once

#include <moya/libmoya/kernel/bsdf/Closure.h>
#include <moya/libmoya/kernel/bsdf/scattering/Diffuse.h>
#include <moya/libmoya/kernel/bsdf/scattering/Ggx.h>
#include <moya/libmoya/kernel/Qualifiers.h>
#include <moya/libmoya/kernel/spectrum/Spectrum.h>

#include <cmath>

#include <glm/geometric.hpp>
#include <glm/vec2.hpp>
#include <glm/vec3.hpp>

namespace v3d::moya::kernel::bsdf::scattering {

/** The least roughness the fits hold for. **/
inline constexpr float kLeastFuzz = 0.01F;

/**
 * OpenPBR's fuzz: Zeltner, Burley and Chiang's sheen of a layer of fibres, in a frame whose z is
 * the shading normal. It reflects on the side light leaves towards, on either side. The closure's
 * colour tints it, and its roughness, held to [kLeastFuzz, 1], spreads it.
 *
 * - **Its value times the cosine** is its albedo at outgoing's cosine times a distribution of
 *   incoming that integrates to one: a clamped cosine, transformed linearly by a matrix fitted to
 *   outgoing's cosine and the roughness. The matrix works in a frame whose x lies along outgoing's
 *   projection onto the surface.
 * - **Its albedo** is a fitted closed form, so a sample's mean weight is its albedo exactly.
 * - **A sample** draws from the distribution exactly, so its density is the distribution.
 * - **It is not reciprocal**, since the distribution is fitted to outgoing alone.
 *
 * The fits are the authors', as MaterialX's library gives them.
 **/
struct Fuzz {
    /** The value for two directions, before the closure's weight. **/
    MOYA_KERNEL static spectrum::SampledSpectrum evaluate(const Closure & closure, glm::vec3 outgoing, glm::vec3 incoming) {
        if (!(outgoing.z * incoming.z > 0.0F)) {
            return spectrum::SampledSpectrum(0.0F);
        }
        upward(outgoing, incoming);
        const float roughness = held(closure.roughness);
        return closure.colour * (albedo(outgoing.z, roughness) * distribution(outgoing, incoming, roughness) / incoming.z);
    }

    /** The density sample() draws incoming with, for outgoing. **/
    MOYA_KERNEL static float density(const Closure & closure, glm::vec3 outgoing, glm::vec3 incoming) {
        if (!(outgoing.z * incoming.z > 0.0F)) {
            return 0.0F;
        }
        upward(outgoing, incoming);
        return distribution(outgoing, incoming, held(closure.roughness));
    }

    /** A direction light comes in from, on outgoing's side, drawn from two random numbers. **/
    MOYA_KERNEL static glm::vec3 sample(const Closure & closure, const glm::vec3 & outgoing, const glm::vec2 & u) {
        const float side = outgoing.z < 0.0F ? -1.0F : 1.0F;
        const glm::vec3 up { outgoing.x, outgoing.y, outgoing.z * side };
        const float roughness = held(closure.roughness);
        const float cosine = std::fmin(up.z, 1.0F);
        const float a = inverseA(cosine, roughness);
        const float b = inverseB(cosine, roughness);
        const glm::vec3 drawn = Diffuse::sample(glm::vec3 { 0.0F, 0.0F, 1.0F }, u);
        const glm::vec3 shaped = glm::normalize(glm::vec3 { (drawn.x - (drawn.z * b)) / a, drawn.y / a, drawn.z });
        const glm::vec3 across = acrossOf(up);
        const glm::vec3 along { -across.y, across.x, 0.0F };
        const glm::vec3 out = (shaped.x * across) + (shaped.y * along) + glm::vec3 { 0.0F, 0.0F, shaped.z };
        return glm::vec3 { out.x, out.y, out.z * side };
    }

    /** Its albedo at a cosine and a roughness, before its colour, held to [0, 1]. **/
    // NOLINTNEXTLINE(bugprone-easily-swappable-parameters) - the cosine, then the roughness
    MOYA_KERNEL static float albedo(float cosine, float roughness) {
        const float x = std::fmin(std::fmax(cosine, 0.0F), 1.0F);
        const float y = held(roughness);
        const float s = y * (0.0206607F + (1.58491F * y)) / (0.0379424F + (y * (1.32227F + y)));
        const float m = y * (-0.193854F + (y * (-1.14885F + (y * (1.7932F - (0.95943F * y * y)))))) / (0.046391F + y);
        const float o = y * (0.000654023F + ((-0.0207818F + (0.119681F * y)) * y)) / (1.26264F + (y * (-1.92021F + y)));
        const float t = (x - m) / s;
        return std::fmin(std::fmax((std::exp(-0.5F * t * t) / (s * std::sqrt(2.0F * kPi))) + o, 0.0F), 1.0F);
    }

 private:
    /** A roughness held to the fits' range. **/
    MOYA_KERNEL static float held(float roughness) {
        return std::fmin(std::fmax(roughness, kLeastFuzz), 1.0F);
    }

    /** Turns two directions on one side to face up. **/
    MOYA_KERNEL static void upward(glm::vec3 & outgoing, glm::vec3 & incoming) {
        if (outgoing.z < 0.0F) {
            outgoing.z = -outgoing.z;
            incoming.z = -incoming.z;
        }
    }

    /** The fitted entries of the inverse matrix: its diagonal, and its corner. **/
    // NOLINTNEXTLINE(bugprone-easily-swappable-parameters) - the cosine, then the roughness
    MOYA_KERNEL static float inverseA(float x, float y) {
        return ((2.58126F * x) + (0.813703F * y)) * y / (1.0F + (0.310327F * x * x) + (2.60994F * x * y));
    }

    // NOLINTNEXTLINE(bugprone-easily-swappable-parameters) - the cosine, then the roughness
    MOYA_KERNEL static float inverseB(float x, float y) {
        return std::sqrt(std::fmax(0.0F, 1.0F - x)) * (y - 1.0F) * y * y * y / (0.0000254053F + (1.71228F * x) - (1.71506F * x * y) + (1.34174F * y * y));
    }

    /** The unit direction of outgoing's projection onto the surface, or x where it has none. **/
    MOYA_KERNEL static glm::vec3 acrossOf(const glm::vec3 & outgoing) {
        const float squared = (outgoing.x * outgoing.x) + (outgoing.y * outgoing.y);
        return squared > 0.0F ? glm::vec3 { outgoing.x, outgoing.y, 0.0F } / std::sqrt(squared) : glm::vec3 { 1.0F, 0.0F, 0.0F };
    }

    /**
     * The distribution at incoming, for outgoing, both above the surface: the clamped cosine at
     * the inverse matrix times incoming, over pi, times the inverse's determinant over the fourth
     * power of that product's length.
     **/
    // NOLINTNEXTLINE(bugprone-easily-swappable-parameters) - the directions, then the roughness
    MOYA_KERNEL static float distribution(const glm::vec3 & outgoing, const glm::vec3 & incoming, float roughness) {
        const float cosine = std::fmin(outgoing.z, 1.0F);
        const float a = inverseA(cosine, roughness);
        const float b = inverseB(cosine, roughness);
        const glm::vec3 across = acrossOf(outgoing);
        const glm::vec3 along { -across.y, across.x, 0.0F };
        const glm::vec3 local { glm::dot(incoming, across), glm::dot(incoming, along), incoming.z };
        const glm::vec3 shaped { (a * local.x) + (b * local.z), a * local.y, local.z };
        const float squared = glm::dot(shaped, shaped);
        if (!(shaped.z > 0.0F) || !(squared > 0.0F)) {
            return 0.0F;
        }
        return shaped.z * kInversePi * (a / squared) * (a / squared);
    }
};

};  // namespace v3d::moya::kernel::bsdf::scattering
