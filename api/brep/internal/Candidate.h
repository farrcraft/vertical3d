/**
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 **/

#pragma once

#include <api/brep/Index.h>

#include <cstddef>

namespace v3d::brep::internal {

/**
 * Two triangles that could be joined into a quad across the edge they share.
 **/
struct Candidate {
    float angle;
    std::size_t first;
    std::size_t second;
    Index from;
    Index to;
};

};  // namespace v3d::brep::internal
