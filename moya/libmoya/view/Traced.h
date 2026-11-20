/*
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 */

#pragma once

#include <moya/libmoya/kernel/geometry/Hit.h>

#include <vector>

namespace v3d::moya::view {

/** Each ray's nearest hit, and whether anything lies on it. **/
struct Traced {
    std::vector<kernel::geometry::Hit> nearest;
    std::vector<bool> any;
};

};  // namespace v3d::moya::view
