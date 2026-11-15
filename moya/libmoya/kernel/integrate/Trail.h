/*
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 */

#pragma once

#include <moya/libmoya/kernel/integrate/Event.h>

#include <cstdint>

namespace v3d::moya::kernel::integrate {

/** The most events a trail keeps of one pixel's path. **/
inline constexpr std::uint32_t kTrailSteps = 16;

/**
 * One event a path recorded in its pixel's trail. used is one once it holds an event. gathered is
 * one for a light whose light the path gathered there, and zero for an event the path went on
 * from: the camera, or a bounce.
 **/
struct Step {
    Event event;
    std::uint32_t used { 0 };
    std::uint32_t gathered { 0 };
};

};  // namespace v3d::moya::kernel::integrate
