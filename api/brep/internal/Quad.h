/**
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 **/

#pragma once

#include <api/brep/Index.h>

namespace v3d::brep::internal {

/** One quad of a ring, and its two half edges that run between a corner and its copy. **/
struct Quad {
    Index face = INVALID_ID;
    Index falling = INVALID_ID;
    Index rising = INVALID_ID;
};

};  // namespace v3d::brep::internal
