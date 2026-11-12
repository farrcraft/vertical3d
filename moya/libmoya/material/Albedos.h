/*
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 */

#pragma once

#include <moya/libmoya/kernel/bsdf/Albedos.h>

namespace v3d::moya::material {

/**
 * The albedos of GGX's single scattering, a reflection's and a dielectric's, as moya_albedo
 * integrated them at build time, which energy compensation reads. The view points at host memory
 * that lives as long as the program. A GPU device copies it into its own.
 **/
kernel::bsdf::Albedos ggxAlbedos();

};  // namespace v3d::moya::material
