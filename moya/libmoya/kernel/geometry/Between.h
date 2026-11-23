/*
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 */

#pragma once

#include <cstdint>

namespace v3d::moya::kernel::geometry {

/**
 * Where a time falls among samples: the sample at or before it, and how far it lies towards the
 * next, from 0 to 1. A time before the first sample is at the first, and one after the last is at
 * the last.
 **/
struct Between {
    std::uint32_t at { 0 };
    float along { 0.0F };
};

};  // namespace v3d::moya::kernel::geometry
