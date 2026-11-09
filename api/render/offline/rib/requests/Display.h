/*
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 */

#pragma once

#include <string>

namespace v3d::render::offline::rib::requests {

/**
 * RiDisplay: where the image goes, what kind of output it is, and which channels it holds.
 **/
struct Display {
    std::string name;
    std::string type;
    std::string mode;
};

};  // namespace v3d::render::offline::rib::requests
