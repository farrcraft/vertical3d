/*
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 */

#pragma once

#include <moya/libmoya/kernel/bsdf/Model.h>

#include <cstdint>

#include <glm/vec3.hpp>

namespace v3d::moya::kernel::surface {

/** The index of refraction of common glass, a dielectric closure's unless code sets another. **/
inline constexpr float kGlassIor = 1.5F;

/**
 * A closure a material names: its model and its scale, and what a model beyond the diffuse one
 * reads. A diffuse closure or a mirror weighs its scale times the surface's colour. A conductor
 * weighs its scale, reflects the surface's colour at normal incidence, and takes its roughness,
 * its anisotropy and its F82 tint, in ACEScg, as OpenPBR names them. A dielectric weighs its
 * scale, and takes its roughness, its anisotropy, the index of refraction inside it, and whether
 * it is thin-walled.
 *
 * It holds no padding, so two closures of the same values hold the same bytes: the bytes after the
 * model are a member of their own, always zero, and thin-walled is a whole word.
 **/
struct MaterialClosure {
    bsdf::Model model { bsdf::Model::Diffuse };
    std::uint8_t unused[3] {};
    float scale { 1.0F };
    float roughness { 0.0F };
    float anisotropy { 0.0F };
    glm::vec3 tint { 1.0F };
    float ior { kGlassIor };

    /** Whether a dielectric is thin-walled: 1 when it is, and 0 when it is not. **/
    std::uint32_t thin { 0 };
};

};  // namespace v3d::moya::kernel::surface
