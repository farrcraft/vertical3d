/**
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 **/

#pragma once

#include <cstddef>

namespace v3d::animation::internal {

/** The key a segment starts at, how far through it to sample, and how long it lasts. **/
struct Segment {
    std::size_t key = 0;
    float u = 0.0F;
    float span = 0.0F;
};

};  // namespace v3d::animation::internal
