/*
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 */

#pragma once

#include <moya/libmoya/kernel/bsdf/Model.h>

namespace v3d::moya::kernel::surface {

/** A closure a material names: its model, and the share of the surface's colour it weighs. **/
struct MaterialClosure {
    bsdf::Model model { bsdf::Model::Diffuse };
    float scale { 1.0F };
};

};  // namespace v3d::moya::kernel::surface
