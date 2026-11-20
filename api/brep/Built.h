/*
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 */

#pragma once

#include <cstddef>
#include <string>

#include "BRep.h"

#include <boost/shared_ptr.hpp>

namespace v3d::brep {

/**
 * What build() made.
 *
 * mesh is null when the input was refused, and problem then says why. skipped counts the faces
 * left out because they had fewer than three distinct corners, which files hold where a
 * triangle has collapsed to a line or a point.
 **/
struct Built {
    boost::shared_ptr<BRep> mesh;
    std::string problem;
    std::size_t skipped = 0;
};

};  // namespace v3d::brep
