/*
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 */

#pragma once

#include <cstdint>
#include <limits>

#include <glm/vec3.hpp>

namespace v3d::moya::kernel::surface {

/** The material of an instance that names none. **/
inline constexpr std::uint32_t kNoMaterial = std::numeric_limits<std::uint32_t>::max();

/**
 * A material as shading reads it: the colour its surface node gives, in ACEScg, when it gives one.
 * The material program replaces it.
 **/
struct Material {
    glm::vec3 colour { 0.5F };

    /** One when the surface node gives a colour. **/
    std::uint32_t coloured { 0 };
};

};  // namespace v3d::moya::kernel::surface
