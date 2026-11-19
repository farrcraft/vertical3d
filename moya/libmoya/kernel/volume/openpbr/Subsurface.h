/*
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 */

#pragma once

#include <moya/libmoya/kernel/Qualifiers.h>
#include <moya/libmoya/kernel/surface/OpenPbr.h>
#include <moya/libmoya/kernel/volume/Lobe.h>
#include <moya/libmoya/kernel/volume/Medium.h>

#include <cmath>

#include <glm/common.hpp>
#include <glm/vec3.hpp>

namespace v3d::moya::kernel::volume::openpbr {

/** The least mean free path a subsurface is taken at, so its extinction stays finite. **/
inline constexpr float kLeastFreePath = 1.0e-6F;

/**
 * The single-scattering albedo of a medium seen as a colour, by the mapping OpenPBR gives. It is
 * one less s squared, over one less the anisotropy times s squared. s is 4.09712 plus 4.20863
 * times the colour, less the root of 9.59217 plus 41.6808 times the colour plus 17.7126 times its
 * square. The colour and the albedo are held to [0, 1], and the anisotropy to [-1, 1].
 **/
// NOLINTNEXTLINE(bugprone-easily-swappable-parameters) - the colour, then the anisotropy
MOYA_KERNEL inline float singleAlbedo(float colour, float anisotropy) {
    const float held = std::fmin(std::fmax(colour, 0.0F), 1.0F);
    const float s = 4.09712F + (4.20863F * held) - std::sqrt(9.59217F + (41.6808F * held) + (17.7126F * held * held));  // NOLINT(readability-magic-numbers) - OpenPBR's fit
    const float squared = s * s;
    const float forwards = std::fmin(std::fmax(anisotropy, -1.0F), 1.0F);
    return std::fmin(std::fmax((1.0F - squared) / (1.0F - (forwards * squared)), 0.0F), 1.0F);
}

/**
 * The medium below an OpenPBR surface whose subsurface weight is above zero, which a path walks
 * between entering and leaving the surface. Its mean free path in each ACEScg channel is the
 * radius times the radius scale, and its extinction one over that. Its scattering is the
 * extinction times the single-scattering albedo singleAlbedo() gives the subsurface colour, and
 * its absorption the rest. The one lobe scatters by the subsurface's anisotropy. A surface of no
 * subsurface weight, or a thin wall, holds no such medium.
 **/
MOYA_KERNEL inline Medium walked(const surface::OpenPbr & surface) {
    Medium out;
    if (surface.thinWalled != 0U || !(surface.subsurfaceWeight > 0.0F)) {
        return out;
    }
    const glm::vec3 path = glm::max(glm::vec3(std::fmax(surface.subsurfaceRadius, 0.0F)) * surface.subsurfaceRadiusScale, glm::vec3(kLeastFreePath));
    const glm::vec3 extinct = glm::vec3(1.0F) / path;
    const float forwards = glm::clamp(surface.subsurfaceScatterAnisotropy, -1.0F, 1.0F);
    const glm::vec3 albedo { singleAlbedo(surface.subsurfaceColour.x, forwards), singleAlbedo(surface.subsurfaceColour.y, forwards),
        singleAlbedo(surface.subsurfaceColour.z, forwards) };
    const glm::vec3 scattering = extinct * albedo;
    out.absorption = extinct - scattering;
    out.lobes[0] = Lobe { .scattering = scattering, .anisotropy = forwards };
    return out;
}

};  // namespace v3d::moya::kernel::volume::openpbr
