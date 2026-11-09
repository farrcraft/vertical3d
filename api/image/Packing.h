/*
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 */

#pragma once

#include <string>
#include <vector>

#include <boost/shared_ptr.hpp>

#include "Image.h"
#include "Placement.h"

namespace v3d::image {

/**
 * What pack() made: the sheet and where each sprite went, or the reason there is no sheet.
 **/
class Packing final {
 public:
    /** Null when the sprites were refused. **/
    boost::shared_ptr<Image> sheet;
    /** One for each sprite, in the order the sprites were given. **/
    std::vector<Placement> placements;
    /** Empty when there is a sheet. **/
    std::string refusal;
};

};  // namespace v3d::image
