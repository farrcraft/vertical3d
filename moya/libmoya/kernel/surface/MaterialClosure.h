/*
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 */

#pragma once

#include <moya/libmoya/kernel/bsdf/Model.h>

#include <glm/vec3.hpp>

namespace v3d::moya::kernel::surface {

/**
 * A closure a material names: its model and its scale, and what a model beyond the diffuse one
 * reads. A diffuse closure or a mirror weighs its scale times the surface's colour. A conductor
 * weighs its scale, reflects the surface's colour at normal incidence, and takes its roughness,
 * its anisotropy and its F82 tint, in ACEScg, as OpenPBR names them.
 **/
struct MaterialClosure {
    bsdf::Model model { bsdf::Model::Diffuse };
    float scale { 1.0F };
    float roughness { 0.0F };
    float anisotropy { 0.0F };
    glm::vec3 tint { 1.0F };
};

};  // namespace v3d::moya::kernel::surface
