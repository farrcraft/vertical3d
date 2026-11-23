/*
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 */

#pragma once

#include <moya/libmoya/kernel/program/Parameters.h>
#include <moya/libmoya/kernel/program/Program.h>
#include <moya/libmoya/kernel/program/Run.h>
#include <moya/libmoya/kernel/Qualifiers.h>
#include <moya/libmoya/kernel/surface/Hair.h>
#include <moya/libmoya/kernel/surface/ShadingPoint.h>
#include <moya/libmoya/kernel/surface/Surfaces.h>
#include <moya/libmoya/kernel/texture/Texels.h>

#include <cstdint>

#include <glm/vec2.hpp>

namespace v3d::moya::kernel::program {

/**
 * The first register of each of a hair's parameters that a hair's program gives, in the order
 * surface::Hair holds them. A colour and the absorption take three registers, a roughness two,
 * and every other parameter one.
 **/
inline constexpr std::uint32_t kHairTintR = 0;
inline constexpr std::uint32_t kHairTintTT = 3;
inline constexpr std::uint32_t kHairTintTRT = 6;
inline constexpr std::uint32_t kHairIor = 9;
inline constexpr std::uint32_t kHairRoughnessR = 10;
inline constexpr std::uint32_t kHairRoughnessTT = 12;
inline constexpr std::uint32_t kHairRoughnessTRT = 14;
inline constexpr std::uint32_t kHairCuticleAngle = 16;
inline constexpr std::uint32_t kHairAbsorption = 17;

/** The registers a hair's parameters take, which a hair's program's values begin with. **/
inline constexpr std::uint32_t kHairRegisters = 20;

/** A hair's parameters as registers hold them. **/
MOYA_KERNEL inline surface::Hair hairOf(const float * registers) {
    const auto pair = [registers](std::uint32_t first) {
        return glm::vec2 { registers[first], registers[first + 1U] };
    };
    surface::Hair out;
    out.tintR = colourAt(registers, kHairTintR);
    out.tintTT = colourAt(registers, kHairTintTT);
    out.tintTRT = colourAt(registers, kHairTintTRT);
    out.ior = registers[kHairIor];
    out.roughnessR = pair(kHairRoughnessR);
    out.roughnessTT = pair(kHairRoughnessTT);
    out.roughnessTRT = pair(kHairRoughnessTRT);
    out.cuticleAngle = registers[kHairCuticleAngle];
    out.absorption = colourAt(registers, kHairAbsorption);
    return out;
}

/** Runs a hair's program at a shading point, and gives the parameters its registers then hold. **/
template <texture::Texels X>
MOYA_KERNEL surface::Hair evaluateHair(const surface::Surfaces & surfaces, const X & textures, const surface::ShadingPoint & point, const Program & program) {
    float registers[kRegisters] {};
    const std::uint64_t mark = texture::started(textures);
    run(surfaces, textures, point, program, registers);
    texture::finished(textures, texture::Work::Program, mark);
    return hairOf(registers);
}

};  // namespace v3d::moya::kernel::program
