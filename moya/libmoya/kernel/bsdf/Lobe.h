/*
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 */

#pragma once

#include <cstdint>

/** How a surface scatters light: the BSDF the integrator evaluates and samples. **/
namespace v3d::moya::kernel::bsdf {

/** The kind of bounce a sample makes, which a depth limit is kept for. **/
enum class Lobe : std::uint8_t {
    Diffuse,
    Glossy,
    Specular,
    Transmission
};

};  // namespace v3d::moya::kernel::bsdf
