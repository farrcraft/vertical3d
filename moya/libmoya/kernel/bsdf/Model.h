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
 * - Dielectric is the reflection and transmission of an interface between two dielectrics, by
 *   Fresnel's equations, or of a thin slab of one, with the energy a single scattering loses given
 *   back.
 * - OrenNayar is OpenPBR's energy-preserving Oren-Nayar diffuse reflection of a roughness.
 * - Specular is a dielectric's GGX reflection alone, by Fresnel's equations, as a layer over a
 *   base reflects, with the energy a single scattering loses given back.
 * - Fuzz is OpenPBR's fuzz, Zeltner, Burley and Chiang's sheen of a layer of fibres.
 * - Hair is Chiang and others' far-field model of a hair fibre, in a frame whose tangent runs
 *   along the fibre. It scatters by the fibre its BSDF holds.
 **/
enum class Model : std::uint8_t {
    Diffuse,
    Mirror,
    Conductor,
    Dielectric,
    OrenNayar,
    Specular,
    Fuzz,
    Hair
};

};  // namespace v3d::moya::kernel::bsdf
