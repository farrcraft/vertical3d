/**
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 **/

#pragma once

#include <api/brep/Index.h>

namespace v3d::brep::internal {

/** A half edge that keeps a length after a merge: where it now runs, and what follows it. **/
struct Kept {
    Index face = INVALID_ID;
    Index from = INVALID_ID;
    Index to = INVALID_ID;
    Index next = INVALID_ID;
};

};  // namespace v3d::brep::internal
