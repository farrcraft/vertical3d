/**
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 **/

#pragma once

#include <api/brep/BRep.h>
#include <api/brep/Index.h>

namespace v3d::brep::internal {

/** A merge's result: the compacted arrays, and the merged vertex's index in them. **/
struct Merged {
    BRep::Snapshot arrays;
    Index vertex = INVALID_ID;
};

};  // namespace v3d::brep::internal
