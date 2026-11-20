/*
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 */

#pragma once

#include <moya/libmoya/kernel/geometry/Cone.h>
#include <moya/libmoya/kernel/Qualifiers.h>

#include <cmath>

#include <glm/geometric.hpp>
#include <glm/vec3.hpp>

namespace v3d::moya::kernel::surface {

/**
 * The least cosine between a ray and a surface that a footprint is projected at. A ray more
 * grazing than that takes its footprint, which is then at most a hundred times longer than wide.
 **/
inline constexpr float kLeastCosine = 1.0e-2F;

/** The least squared length of a direction across a ray before it is taken as the ray's own. **/
inline constexpr float kLeastAcross = 1.0e-12F;

/**
 * Where a ray's cone meets a surface: an ellipse in the surface's plane, as its two axes, each
 * the ellipse's whole length along it, in world space. The major axis runs along the ray as the
 * surface sees it, and the minor axis across it, as wide as the cone. Axes of no length are a
 * footprint of no area.
 **/
struct Footprint {
    glm::vec3 major { 0.0F };
    glm::vec3 minor { 0.0F };

    /** The cone's width at the surface: the minor axis's length. **/
    MOYA_KERNEL float width() const {
        return glm::length(minor);
    }
};

/**
 * The footprint of a cone of a width meeting a surface along a direction of unit length, in the
 * plane a unit normal gives. A ray along the normal gives a circle.
 **/
MOYA_KERNEL inline Footprint footprintOf(float width, const glm::vec3 & direction, const glm::vec3 & normal) {
    const float cosine = std::fmax(std::fabs(glm::dot(direction, normal)), kLeastCosine);
    glm::vec3 across = glm::cross(direction, normal);
    if (!(glm::dot(across, across) > kLeastAcross)) {
        // straight on: any direction in the plane is across the ray
        const glm::vec3 other = std::fabs(normal.x) < 0.5F ? glm::vec3 { 1.0F, 0.0F, 0.0F } : glm::vec3 { 0.0F, 1.0F, 0.0F };
        across = glm::cross(other, normal);
    }
    across = glm::normalize(across);
    const glm::vec3 along = glm::cross(normal, across);
    return { along * (width / cosine), across * width };
}

/**
 * How much a bounce widens a cone's spread for a lobe's roughness, a tuned rule: the arctangent
 * of the roughness. A diffuse lobe, of roughness one, adds an eighth of a turn.
 **/
MOYA_KERNEL inline float roughWidening(float roughness) {
    return std::atan(std::fmax(roughness, 0.0F));
}

/**
 * The cone that leaves a surface where an arriving one's footprint is of a width. It starts that
 * wide. A reflection's spread grows by twice the surface's curvature times the width, which a
 * convex surface spreads and a concave one focuses. A transmission's keeps the arriving spread.
 * Either grows by the sampled lobe's roughness.
 **/
// NOLINTNEXTLINE(bugprone-easily-swappable-parameters) - the width, then the curvature
MOYA_KERNEL inline geometry::Cone bounced(const geometry::Cone & arriving, float width, float curvature, float roughness, bool crossed) {
    geometry::Cone out;
    out.width = width;
    out.spread = arriving.spread + roughWidening(roughness);
    if (!crossed) {
        out.spread += 2.0F * curvature * width;
    }
    return out;
}

};  // namespace v3d::moya::kernel::surface
