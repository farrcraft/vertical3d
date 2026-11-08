/*
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 */

#pragma once

#include <moya/libmoya/kernel/spectrum/Rgb.h>

/**
 * The colour tables the build fits for moya: data kernel code reads, which host code hands to a
 * device.
 **/
namespace v3d::moya::colour {

/**
 * Jakob and Hanika's table for ACEScg, as moya_rgbtable fitted it at build time. The view points
 * at host memory that lives as long as the program. A GPU device copies it into its own.
 **/
kernel::spectrum::RgbTable acescgTable();

};  // namespace v3d::moya::colour
