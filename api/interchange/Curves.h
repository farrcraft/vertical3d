/*
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 */

#pragma once

#include <api/curve/Curve.h>

#include <string>
#include <vector>

namespace v3d::interchange {

/**
 * Curves in their node's space, each of any type, at one width in the node's units. A format
 * that does not say how wide its curves are leaves the width at zero.
 **/
struct Curves {
    std::string name;
    std::vector<v3d::curve::Curve> curves;
    float width = 0.0F;
};

};  // namespace v3d::interchange
