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

/** The least transmittance a transmission colour is taken at, so its logarithm stays finite. **/
inline constexpr float kLeastTransmittance = 1.0e-6F;

/**
 * The medium inside an OpenPBR surface that is not thin-walled, from its transmission, as OpenPBR
 * gives it. Over a depth d, the extinction is the colour's negative logarithm over d, so white
 * light that travels d takes the colour, and the scattering is the scatter over d. The absorption
 * is what extinction the scattering leaves. Where that falls below zero in a channel, every
 * channel's absorption is raised by the same amount, so the least is zero. The one lobe scatters
 * by the scatter's anisotropy. A depth of zero or less, or a thin wall, holds no medium.
 **/
MOYA_KERNEL inline Medium interior(const surface::OpenPbr & surface) {
    Medium out;
    const float depth = surface.transmissionDepth;
    if (surface.thinWalled != 0U || !(depth > 0.0F)) {
        return out;
    }
    const glm::vec3 colour = glm::clamp(surface.transmissionColour, glm::vec3(kLeastTransmittance), glm::vec3(1.0F));
    const glm::vec3 extinct { -std::log(colour.x) / depth, -std::log(colour.y) / depth, -std::log(colour.z) / depth };
    const glm::vec3 scattering = glm::max(surface.transmissionScatter, glm::vec3(0.0F)) / depth;
    glm::vec3 absorption = extinct - scattering;
    const float least = glm::min(absorption.x, glm::min(absorption.y, absorption.z));
    if (least < 0.0F) {
        absorption -= glm::vec3(least);
    }
    out.absorption = absorption;
    out.lobes[0] = Lobe { .scattering = scattering, .anisotropy = glm::clamp(surface.transmissionScatterAnisotropy, -1.0F, 1.0F) };
    return out;
}

};  // namespace v3d::moya::kernel::volume::openpbr
