/*
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 */

#pragma once

#include <moya/libmoya/kernel/Qualifiers.h>
#include <moya/libmoya/kernel/surface/Frame.h>

#include <cmath>

#include <glm/geometric.hpp>
#include <glm/vec3.hpp>

namespace v3d::moya::kernel::surface {

/** The least cosine to the geometric normal that outgoing's mirror about a normal keeps. **/
inline constexpr float kLeastRise = 0.01F;

/** The share of outgoing's own cosine to the geometric normal its mirror keeps, where less. **/
inline constexpr float kRiseShare = 0.9F;

/** The least squared length a direction is taken to have before it is taken as none. **/
inline constexpr float kLeastDirection = 1.0e-12F;

/** A direction over its length, or nothing where it has none. **/
MOYA_KERNEL inline glm::vec3 unitOrNone(const glm::vec3 & direction) {
    const float squared = glm::dot(direction, direction);
    return squared > kLeastDirection ? direction / std::sqrt(squared) : glm::vec3 { 0.0F };
}

/**
 * A normal of unit length bent towards the geometric normal, just far enough that outgoing's
 * mirror about it rises above the geometric surface, so a surface whose normal a material tilts
 * shows no black. The mirror keeps a cosine to the geometric normal of kLeastRise, or of
 * kRiseShare of outgoing's own where outgoing grazes. A normal whose mirror rises that far is
 * kept, and so is one that lies along the geometric normal.
 *
 * The normal turns in the plane it shares with the geometric normal, so it keeps its direction
 * around it. The normal sought is a sum of the geometric normal and a direction across it, whose
 * mirror of outgoing has exactly the cosine kept. That is a quadratic in the square of its cosine
 * to the geometric normal, and of its two roots the one that does not mirror outgoing to the
 * other side of the plane. The normals are on the geometric normal's side, and the mirror is
 * taken on outgoing's.
 **/
MOYA_KERNEL inline glm::vec3 visible(const glm::vec3 & normal, const glm::vec3 & geometric, const glm::vec3 & outgoing) {
    const float side = glm::dot(outgoing, geometric) < 0.0F ? -1.0F : 1.0F;
    const glm::vec3 up = geometric * side;
    const glm::vec3 given = normal * side;
    const float rise = glm::dot(outgoing, up);
    const float least = std::fmin(kRiseShare * rise, kLeastRise);
    const glm::vec3 mirrored = (2.0F * glm::dot(given, outgoing) * given) - outgoing;
    if (glm::dot(mirrored, up) >= least) {
        return normal;
    }
    const glm::vec3 across = unitOrNone(given - (up * glm::dot(given, up)));
    const float onAcross = glm::dot(outgoing, across);
    const float quartic = (onAcross * onAcross) + (rise * rise);
    if (!(glm::dot(across, across) > 0.0F) || !(quartic > 0.0F)) {
        return normal;
    }
    const float quadratic = 2.0F * (quartic + (rise * least));
    const float constant = (least + rise) * (least + rise);
    const float root = std::sqrt(std::fmax((quadratic * quadratic) - (4.0F * quartic * constant), 0.0F));
    const float squared = std::fmin(std::fmax((onAcross < 0.0F ? quadratic + root : quadratic - root) / (4.0F * quartic), 0.0F), 1.0F);
    return ((across * std::sqrt(1.0F - squared)) + (up * std::sqrt(squared))) * side;
}

/**
 * The frame about a normal and a tangent a material gives, in world space, from the frame a
 * shading point's tangent gives. A normal of no length is the point's own, and a tangent of no
 * length the point's tangent, so a material that gives neither keeps the point's frame.
 *
 * A normal the material gives is made of unit length and bent by visible(). The tangent is laid
 * flat against the normal, or is the frame's own around the normal where it lies along it. The
 * bitangent is the normal's cross product with the tangent, turned to the point's bitangent's
 * side.
 **/
// NOLINTNEXTLINE(bugprone-easily-swappable-parameters) - the material's, then the surface's
MOYA_KERNEL inline Frame frameAbout(const Frame & point, const glm::vec3 & normal, const glm::vec3 & tangent, const glm::vec3 & geometric,
    const glm::vec3 & outgoing) {  // NOLINT(bugprone-easily-swappable-parameters)
    const glm::vec3 givenNormal = unitOrNone(normal);
    const glm::vec3 givenTangent = unitOrNone(tangent);
    const bool hasNormal = glm::dot(givenNormal, givenNormal) > 0.0F;
    if (!hasNormal && !(glm::dot(givenTangent, givenTangent) > 0.0F)) {
        return point;
    }
    Frame out;
    out.normal = hasNormal ? visible(givenNormal, geometric, outgoing) : point.normal;
    const glm::vec3 along = glm::dot(givenTangent, givenTangent) > 0.0F ? givenTangent : point.tangent;
    out.tangent = unitOrNone(along - (out.normal * glm::dot(out.normal, along)));
    if (!(glm::dot(out.tangent, out.tangent) > 0.0F)) {
        out.tangent = Frame::around(out.normal).tangent;
    }
    out.bitangent = glm::cross(out.normal, out.tangent);
    if (glm::dot(out.bitangent, point.bitangent) < 0.0F) {
        out.bitangent = -out.bitangent;
    }
    return out;
}

};  // namespace v3d::moya::kernel::surface
