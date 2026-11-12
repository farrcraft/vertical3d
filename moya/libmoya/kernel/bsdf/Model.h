/*
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 */

#pragma once

#include <cstdint>

namespace v3d::moya::kernel::bsdf {

/**
 * The scattering model a closure holds. Kernel code has no virtual call, so a BSDF switches over
 * it.
 *
 * - Diffuse is Lambert's reflection.
 * - Mirror is perfect specular reflection, about the shading normal.
 * - Conductor is a metal's GGX reflection by its F82 Fresnel factor, with the energy a single
 *   scattering loses given back.
 **/
enum class Model : std::uint8_t {
    Diffuse,
    Mirror,
    Conductor
};

};  // namespace v3d::moya::kernel::bsdf
