/*
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 */

#pragma once

#include <moya/libmoya/kernel/program/Parameters.h>
#include <moya/libmoya/kernel/Qualifiers.h>
#include <moya/libmoya/kernel/volume/Lobe.h>
#include <moya/libmoya/kernel/volume/Medium.h>

#include <cstdint>

namespace v3d::moya::kernel::program {

/**
 * The first register of each of a medium's values that a volume's program gives: the absorption,
 * then each lobe's scattering and anisotropy, then the emission. A coefficient takes three
 * registers and an anisotropy one.
 **/
inline constexpr std::uint32_t kMediumAbsorption = 0;
inline constexpr std::uint32_t kMediumLobes = 3;
inline constexpr std::uint32_t kLobeRegisters = 4;
inline constexpr std::uint32_t kMediumEmission = kMediumLobes + (volume::kMostLobes * kLobeRegisters);

/** The registers a medium's values take, which a volume's program's values begin with. **/
inline constexpr std::uint32_t kMediumRegisters = kMediumEmission + 3U;

/** The first register of a lobe's scattering, and of its anisotropy. **/
MOYA_KERNEL constexpr std::uint32_t lobeScattering(std::uint32_t lobe) {
    return kMediumLobes + (lobe * kLobeRegisters);
}

MOYA_KERNEL constexpr std::uint32_t lobeAnisotropy(std::uint32_t lobe) {
    return lobeScattering(lobe) + 3U;
}

/** A medium as registers hold it. **/
MOYA_KERNEL inline volume::Medium mediumOf(const float * registers) {
    volume::Medium out;
    out.absorption = colourAt(registers, kMediumAbsorption);
    for (std::uint32_t lobe = 0; lobe < volume::kMostLobes; ++lobe) {
        out.lobes[lobe] = volume::Lobe { .scattering = colourAt(registers, lobeScattering(lobe)), .anisotropy = registers[lobeAnisotropy(lobe)] };
    }
    out.emission = colourAt(registers, kMediumEmission);
    return out;
}

};  // namespace v3d::moya::kernel::program
