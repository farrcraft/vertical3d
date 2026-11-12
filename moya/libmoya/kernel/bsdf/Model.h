/*
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 */

#pragma once

#include <moya/libmoya/kernel/bsdf/Lobe.h>
#include <moya/libmoya/kernel/Qualifiers.h>

#include <cstdint>

namespace v3d::moya::kernel::bsdf {

/**
 * The scattering model a closure holds. Kernel code has no virtual call, so a BSDF switches over
 * it.
 *
 * - Diffuse is Lambert's reflection.
 * - Mirror is perfect specular reflection, about the shading normal.
 **/
enum class Model : std::uint8_t {
    Diffuse,
    Mirror
};

/** The kind of bounce a sample of a model makes. **/
MOYA_KERNEL constexpr Lobe lobeOf(Model model) {
    return model == Model::Mirror ? Lobe::Specular : Lobe::Diffuse;
}

/**
 * Whether a model scatters into a single direction for each outgoing one. Such a model has no
 * value and no density for any direction it is given, and only its sample finds its light.
 **/
MOYA_KERNEL constexpr bool singular(Model model) {
    return model == Model::Mirror;
}

};  // namespace v3d::moya::kernel::bsdf
