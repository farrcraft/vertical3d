/*
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 */

#pragma once

#include <moya/libmoya/kernel/Qualifiers.h>
#include <moya/libmoya/kernel/surface/OpenPbr.h>

#include <cstdint>

#include <glm/vec3.hpp>

namespace v3d::moya::kernel::program {

/**
 * The first register of each of OpenPBR's parameters that a program gives, in the order
 * surface::OpenPbr holds them. A colour takes three registers, and every other parameter one.
 **/
inline constexpr std::uint32_t kBaseWeight = 0;
inline constexpr std::uint32_t kBaseColour = 1;
inline constexpr std::uint32_t kBaseDiffuseRoughness = 4;
inline constexpr std::uint32_t kBaseMetalness = 5;
inline constexpr std::uint32_t kSpecularWeight = 6;
inline constexpr std::uint32_t kSpecularColour = 7;
inline constexpr std::uint32_t kSpecularRoughness = 10;
inline constexpr std::uint32_t kSpecularAnisotropy = 11;
inline constexpr std::uint32_t kSpecularIor = 12;
inline constexpr std::uint32_t kTransmissionWeight = 13;
inline constexpr std::uint32_t kTransmissionColour = 14;
inline constexpr std::uint32_t kSubsurfaceWeight = 17;
inline constexpr std::uint32_t kSubsurfaceColour = 18;
inline constexpr std::uint32_t kCoatWeight = 21;
inline constexpr std::uint32_t kCoatColour = 22;
inline constexpr std::uint32_t kCoatRoughness = 25;
inline constexpr std::uint32_t kCoatAnisotropy = 26;
inline constexpr std::uint32_t kCoatIor = 27;
inline constexpr std::uint32_t kCoatDarkening = 28;
inline constexpr std::uint32_t kFuzzWeight = 29;
inline constexpr std::uint32_t kFuzzColour = 30;
inline constexpr std::uint32_t kFuzzRoughness = 33;
inline constexpr std::uint32_t kThinFilmWeight = 34;
inline constexpr std::uint32_t kThinFilmThickness = 35;
inline constexpr std::uint32_t kThinFilmIor = 36;
inline constexpr std::uint32_t kEmissionLuminance = 37;
inline constexpr std::uint32_t kEmissionColour = 38;

/** The registers the parameters take, which every program's values begin with. **/
inline constexpr std::uint32_t kParameterRegisters = 41;

/** A colour held in three registers from a first. **/
MOYA_KERNEL inline glm::vec3 colourAt(const float * registers, std::uint32_t first) {
    return glm::vec3 { registers[first], registers[first + 1U], registers[first + 2U] };
}

/** OpenPBR's parameters as registers hold them, and a surface that is not thin-walled. **/
MOYA_KERNEL inline surface::OpenPbr parameters(const float * registers) {
    surface::OpenPbr out;
    out.baseWeight = registers[kBaseWeight];
    out.baseColour = colourAt(registers, kBaseColour);
    out.baseDiffuseRoughness = registers[kBaseDiffuseRoughness];
    out.baseMetalness = registers[kBaseMetalness];
    out.specularWeight = registers[kSpecularWeight];
    out.specularColour = colourAt(registers, kSpecularColour);
    out.specularRoughness = registers[kSpecularRoughness];
    out.specularAnisotropy = registers[kSpecularAnisotropy];
    out.specularIor = registers[kSpecularIor];
    out.transmissionWeight = registers[kTransmissionWeight];
    out.transmissionColour = colourAt(registers, kTransmissionColour);
    out.subsurfaceWeight = registers[kSubsurfaceWeight];
    out.subsurfaceColour = colourAt(registers, kSubsurfaceColour);
    out.coatWeight = registers[kCoatWeight];
    out.coatColour = colourAt(registers, kCoatColour);
    out.coatRoughness = registers[kCoatRoughness];
    out.coatAnisotropy = registers[kCoatAnisotropy];
    out.coatIor = registers[kCoatIor];
    out.coatDarkening = registers[kCoatDarkening];
    out.fuzzWeight = registers[kFuzzWeight];
    out.fuzzColour = colourAt(registers, kFuzzColour);
    out.fuzzRoughness = registers[kFuzzRoughness];
    out.thinFilmWeight = registers[kThinFilmWeight];
    out.thinFilmThickness = registers[kThinFilmThickness];
    out.thinFilmIor = registers[kThinFilmIor];
    out.emissionLuminance = registers[kEmissionLuminance];
    out.emissionColour = colourAt(registers, kEmissionColour);
    return out;
}

};  // namespace v3d::moya::kernel::program
