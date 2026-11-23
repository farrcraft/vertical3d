/*
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 */

#pragma once

#include <vector>

namespace v3d::rib::requests {

/**
 * RiCurves: curves of control points, each a run of the "P" array. A linear curve is straight
 * lines through its points. A cubic curve is evaluated through the current basis in v. A
 * periodic curve joins its end to its start.
 **/
struct Curves {
    bool cubic = false;
    std::vector<unsigned int> vertices;  /**< how many control points each curve has **/
    bool periodic = false;
};

};  // namespace v3d::rib::requests
