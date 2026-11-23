/*
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 */

#pragma once

#include <moya/libmoya/kernel/program/Parameters.h>
#include <moya/libmoya/kernel/program/Program.h>
#include <moya/libmoya/kernel/program/Run.h>
#include <moya/libmoya/kernel/Qualifiers.h>
#include <moya/libmoya/kernel/surface/ShadingPoint.h>
#include <moya/libmoya/kernel/surface/Surfaces.h>
#include <moya/libmoya/kernel/texture/Texels.h>

#include <cstdint>

#include <glm/vec3.hpp>

namespace v3d::moya::kernel::program {

/**
 * The first register of each of a displacement's values that its program gives. The vector takes
 * three registers, along the surface's tangent, its bitangent and its normal, and the scale one.
 * A displacement of a float gives the third of the vector, along the normal, and the other two
 * are zero.
 **/
inline constexpr std::uint32_t kDisplacementVector = 0;
inline constexpr std::uint32_t kDisplacementScale = 3;

/** The registers a displacement's values take, which its program's values begin with. **/
inline constexpr std::uint32_t kDisplacementRegisters = 4;

/** A displacement's vector times its scale, as registers hold them. **/
MOYA_KERNEL inline glm::vec3 displacementOf(const float * registers) {
    return colourAt(registers, kDisplacementVector) * registers[kDisplacementScale];
}

/**
 * Runs a displacement's program at a shading point, and gives the displacement its registers
 * hold after it: along the point's tangent, its bitangent and its normal.
 **/
template <texture::Texels X>
MOYA_KERNEL glm::vec3 displacement(const surface::Surfaces & surfaces, const X & textures, const surface::ShadingPoint & point, const Program & program) {
    float registers[kRegisters] {};
    run(surfaces, textures, point, program, registers);
    return displacementOf(registers);
}

};  // namespace v3d::moya::kernel::program
