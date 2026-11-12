/*
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 */

#pragma once

#include <moya/libmoya/kernel/surface/MaterialClosure.h>

#include <cstdint>
#include <limits>

#include <glm/vec3.hpp>

namespace v3d::moya::kernel::surface {

/** The material of an instance that names none. **/
inline constexpr std::uint32_t kNoMaterial = std::numeric_limits<std::uint32_t>::max();

/** The most closures a material names. **/
inline constexpr std::uint32_t kMostMaterialClosures = 8;

/**
 * A material as shading reads it: the colour its surface node gives, in ACEScg, when it gives
 * one, and the closures its BSDF holds. Each closure's weight is its scale times the surface's
 * colour. A material is one diffuse closure of scale one unless code sets others.
 **/
struct Material {
    glm::vec3 colour { 0.5F };

    /** One when the surface node gives a colour. **/
    std::uint32_t coloured { 0 };

    MaterialClosure closures[kMostMaterialClosures] {};
    std::uint32_t count { 1 };
};

};  // namespace v3d::moya::kernel::surface
