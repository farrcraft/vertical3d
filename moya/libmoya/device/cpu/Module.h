/*
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 */

#pragma once

#include <moya/libmoya/device/cpu/Isa.h>
#include <moya/libmoya/device/cpu/Traversal.h>
#include <moya/libmoya/kernel/Launch.h>
#include <moya/libmoya/kernel/geometry/Trace.h>
#include <moya/libmoya/kernel/integrate/Paths.h>
#include <moya/libmoya/kernel/view/HitView.h>

#include <cstdint>

namespace v3d::moya::device::cpu {

/** What the hit view's entry point reads. **/
using HitParams = kernel::view::HitView<Traversal>;

/** What the entry point that traces a list of rays reads and writes. **/
using TraceParams = kernel::geometry::TraceRays<Traversal>;

/** What the integrator's entry point reads. **/
using PathParams = kernel::integrate::Paths<Traversal>;

/** The version of the table below. A library of another version is refused. **/
inline constexpr std::uint32_t kModuleVersion = 8;

/** The name of the function each library exports, which gives its table. **/
inline constexpr const char * kModuleEntry = "moya_cpu_module";

/**
 * A library's table of entry points, each a kernel function a launch runs for every pixel. Every
 * type a parameter holds compiles alike for every instruction set, so libmoya and each library
 * agree on its layout.
 **/
struct Module {
    std::uint32_t version { 0 };
    Isa isa { Isa::Sse42 };
    kernel::Function<HitParams> hits { nullptr };
    kernel::Function<TraceParams> trace { nullptr };
    kernel::Function<PathParams> paths { nullptr };
};

/** The type of the function each library exports. **/
using ModuleEntry = const Module * (*)();

};  // namespace v3d::moya::device::cpu
