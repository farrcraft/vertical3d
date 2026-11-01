/**
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 **/

#pragma once

#include <api/render/offline/Film.h>

#include <vector>

#include "Spread.h"

namespace v3d::moya::raytrace {

/**
 * One pixel of a row as the ray hider casts it: the samples it has taken so far, in the order
 * it took them, and whether it needs no more.
 **/
class Pixel final {
 public:
    Spread spread;
    unsigned int taken = 0;
    bool settled = false;
    std::vector<v3d::render::offline::Film::Sample> samples;
};

};  // namespace v3d::moya::raytrace
