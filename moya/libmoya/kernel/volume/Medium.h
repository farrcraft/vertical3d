/*
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 */

#pragma once

#include <moya/libmoya/kernel/Qualifiers.h>
#include <moya/libmoya/kernel/volume/Lobe.h>

#include <cstdint>

#include <glm/vec3.hpp>

namespace v3d::moya::kernel::volume {

/** The most lobes of Henyey and Greenstein's phase function a medium scatters by. **/
inline constexpr std::uint32_t kMostLobes = 2;

/**
 * How a medium absorbs, scatters and emits light, each a coefficient per unit of length in each
 * ACEScg channel. Emission is the radiance it adds over each unit of length. A medium of no
 * coefficients is empty.
 **/
struct Medium {
    glm::vec3 absorption { 0.0F };
    Lobe lobes[kMostLobes] {};
    glm::vec3 emission { 0.0F };
};

/** A medium's extinction: what it absorbs and what each of its lobes scatters. **/
MOYA_KERNEL inline glm::vec3 extinction(const Medium & medium) {
    glm::vec3 out = medium.absorption;
    for (const Lobe & lobe : medium.lobes) {
        out += lobe.scattering;
    }
    return out;
}

/** Whether a medium absorbs, scatters or emits nothing at all. **/
MOYA_KERNEL inline bool empty(const Medium & medium) {
    const glm::vec3 all = extinction(medium) + medium.emission;
    return all.x == 0.0F && all.y == 0.0F && all.z == 0.0F;
}

};  // namespace v3d::moya::kernel::volume
