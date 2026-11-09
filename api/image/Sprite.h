/*
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 */

#pragma once

#include <string>

#include <boost/shared_ptr.hpp>

#include "Image.h"

namespace v3d::image {

/**
 * One image to pack into a sheet, or one cut back out of it, and the name it goes under.
 **/
class Sprite final {
 public:
    std::string name;
    boost::shared_ptr<Image> image;
};

};  // namespace v3d::image
