/*
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 */

#pragma once

#include <moya/libmoya/kernel/Qualifiers.h>

#include <cmath>

#include <glm/geometric.hpp>
#include <glm/vec2.hpp>
#include <glm/vec3.hpp>

namespace v3d::moya::kernel::bsdf::scattering {

/** Pi. **/
inline constexpr float kPi = 3.14159265F;

/** The alpha along both axes below which a microfacet lobe is smooth. **/
inline constexpr float kSmoothest = 1.0e-3F;

/**
 * Four. A reflected direction's density is its normal's over four times their cosine, and a
 * reflection's value is over four times the two directions' cosines.
 **/
inline constexpr float kReflection = 4.0F;

/** The least alpha a microfacet lobe takes along either axis. **/
inline constexpr float kLeast = 1.0e-4F;

/**
 * The GGX distribution of microfacet normals, in a frame whose z is the shading normal and whose
 * x is the tangent. Its alpha along x and along y are its roughness along each. Directions lie
 * above the surface, with z above zero.
 *
 * Masking and shadowing are Smith's, height-correlated. Normals are sampled from those visible
 * from a direction, by Heitz's method.
 **/
struct Ggx {
    glm::vec2 alpha { 0.5F, 0.5F };

    /**
     * The alphas of a roughness and an anisotropy, as OpenPBR maps them. The alpha is the square
     * of the roughness. An anisotropy above zero stretches it along the tangent and shrinks it
     * along the bitangent, keeping the area of the highlight. Neither alpha falls below kLeast,
     * where the distribution's arithmetic still holds.
     **/
    // NOLINTNEXTLINE(bugprone-easily-swappable-parameters) - the roughness, then the anisotropy
    MOYA_KERNEL static glm::vec2 alphas(float roughness, float anisotropy) {
        const float alpha = roughness * roughness;
        const float across = 1.0F - anisotropy;
        const float along = alpha * std::sqrt(2.0F / (1.0F + (across * across)));
        return glm::vec2 { std::fmax(along, kLeast), std::fmax(across * along, kLeast) };
    }

    /** Whether alphas this small make the lobe smooth: a mirror, with no microfacets. **/
    MOYA_KERNEL static bool smooth(const glm::vec2 & alpha) {
        return alpha.x < kSmoothest && alpha.y < kSmoothest;
    }

    /** The density of microfacets whose normal is a direction, over projected area. **/
    MOYA_KERNEL float distribution(const glm::vec3 & normal) const {
        const float x = normal.x / alpha.x;
        const float y = normal.y / alpha.y;
        const float sum = (x * x) + (y * y) + (normal.z * normal.z);
        return 1.0F / (kPi * alpha.x * alpha.y * sum * sum);
    }

    /** Smith's lambda for a direction: the share of microfacets it sees from behind. **/
    MOYA_KERNEL float lambda(const glm::vec3 & direction) const {
        const float z = direction.z * direction.z;
        if (!(z > 0.0F)) {
            return 0.0F;
        }
        const float x = alpha.x * direction.x;
        const float y = alpha.y * direction.y;
        return 0.5F * (std::sqrt(1.0F + (((x * x) + (y * y)) / z)) - 1.0F);
    }

    /** The share of microfacets a direction sees. **/
    MOYA_KERNEL float masking(const glm::vec3 & direction) const {
        return 1.0F / (1.0F + lambda(direction));
    }

    /** The share of microfacets both directions see, height-correlated. **/
    MOYA_KERNEL float shadowing(const glm::vec3 & outgoing, const glm::vec3 & incoming) const {
        return 1.0F / (1.0F + lambda(outgoing) + lambda(incoming));
    }

    /** The density of visible normals from outgoing, at a normal. **/
    MOYA_KERNEL float visible(const glm::vec3 & outgoing, const glm::vec3 & normal) const {
        const float facing = glm::dot(outgoing, normal);
        if (!(facing > 0.0F) || !(outgoing.z > 0.0F)) {
            return 0.0F;
        }
        return masking(outgoing) * facing * distribution(normal) / outgoing.z;
    }

    /** A normal visible from outgoing, drawn by its visible density from two numbers. **/
    MOYA_KERNEL glm::vec3 sampleVisible(const glm::vec3 & outgoing, const glm::vec2 & u) const {
        // the view stretched into the hemisphere's configuration, where the distribution is a
        // unit one, and a point drawn on the projected disc it sees
        const glm::vec3 view = glm::normalize(glm::vec3 { alpha.x * outgoing.x, alpha.y * outgoing.y, outgoing.z });
        const float squared = (view.x * view.x) + (view.y * view.y);
        const glm::vec3 first = squared > 0.0F ? glm::vec3 { -view.y, view.x, 0.0F } / std::sqrt(squared) : glm::vec3 { 1.0F, 0.0F, 0.0F };
        const glm::vec3 second = glm::cross(view, first);
        const float radius = std::sqrt(u.x);
        const float angle = 2.0F * kPi * u.y;
        const float t1 = radius * std::cos(angle);
        const float half = 0.5F * (1.0F + view.z);
        const float t2 = ((1.0F - half) * std::sqrt(std::fmax(0.0F, 1.0F - (t1 * t1)))) + (half * radius * std::sin(angle));
        const glm::vec3 stretched = (t1 * first) + (t2 * second) + (std::sqrt(std::fmax(0.0F, 1.0F - (t1 * t1) - (t2 * t2))) * view);
        return glm::normalize(glm::vec3 { alpha.x * stretched.x, alpha.y * stretched.y, std::fmax(0.0F, stretched.z) });
    }

    /** The density of a reflected direction, drawn through a visible normal, for outgoing. **/
    MOYA_KERNEL float reflectedDensity(const glm::vec3 & outgoing, const glm::vec3 & incoming) const {
        const glm::vec3 sum = outgoing + incoming;
        if (!(glm::dot(sum, sum) > 0.0F)) {
            return 0.0F;
        }
        const glm::vec3 normal = glm::normalize(sum);
        const float facing = glm::dot(outgoing, normal);
        return facing > 0.0F ? visible(outgoing, normal) / (kReflection * facing) : 0.0F;
    }

    /** Its reflection's value for a Fresnel factor of one, both directions above the surface. **/
    MOYA_KERNEL float reflected(const glm::vec3 & outgoing, const glm::vec3 & incoming) const {
        const glm::vec3 sum = outgoing + incoming;
        if (!(outgoing.z > 0.0F) || !(incoming.z > 0.0F) || !(glm::dot(sum, sum) > 0.0F)) {
            return 0.0F;
        }
        const glm::vec3 normal = glm::normalize(sum);
        return distribution(normal) * shadowing(outgoing, incoming) / (kReflection * outgoing.z * incoming.z);
    }
};

};  // namespace v3d::moya::kernel::bsdf::scattering
