/*
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 */

#pragma once

#include <cstdint>

namespace v3d::moya::kernel::geometry {

/**
 * Where a geometry's values are held: count values for each of its samples, one sample after
 * another from first, at the times from times on. A geometry of one sample holds it at every time.
 **/
struct Sampled {
    std::uint32_t first { 0 };
    std::uint32_t count { 0 };
    std::uint32_t times { 0 };
    std::uint32_t samples { 1 };
};

};  // namespace v3d::moya::kernel::geometry
