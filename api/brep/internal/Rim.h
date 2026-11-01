/**
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 **/

#pragma once

#include <api/brep/Index.h>

#include <cstddef>

namespace v3d::brep::internal {

/**
 * One half edge of a region's outline: a half edge of a face in the region whose other side is
 * not in the region.
 **/
struct Rim {
    Index edge = INVALID_ID;
    Index from = INVALID_ID;
    Index to = INVALID_ID;
    // the half edge on the other side, or INVALID_ID on the mesh's boundary
    Index outside = INVALID_ID;
    // the half edge of the outline that leaves where this one ends
    Index after = INVALID_ID;
    std::size_t region = 0;
};

};  // namespace v3d::brep::internal
