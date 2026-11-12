/*
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 */

#pragma once

#include <moya/libmoya/kernel/bsdf/Albedos.h>

namespace v3d::moya::material {

/**
 * The albedo of a GGX reflection with a Fresnel factor of one, as moya_albedo integrated it at
 * build time, which energy compensation reads. The view points at host memory that lives as long
 * as the program. A GPU device copies it into its own.
 **/
kernel::bsdf::Albedos ggxAlbedos();

};  // namespace v3d::moya::material
