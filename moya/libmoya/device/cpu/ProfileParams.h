/*
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 */

#pragma once

#include <moya/libmoya/device/cpu/Tally.h>
#include <moya/libmoya/device/cpu/Traversal.h>
#include <moya/libmoya/kernel/Array.h>
#include <moya/libmoya/kernel/integrate/Paths.h>

namespace v3d::moya::device::cpu {

/**
 * What the profiled integrator's entry point reads: the integrator's parameters, and a tally for
 * each pixel, row by row, that its rays and ticks are added to.
 **/
struct ProfileParams {
    kernel::integrate::Paths<Traversal> paths;
    kernel::Array<Tally> tallies;
};

};  // namespace v3d::moya::device::cpu
