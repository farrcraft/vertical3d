/*
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 */

#pragma once

#include <vector>

namespace v3d::moya::tools::rgbtable {

/** What moya_rgbtable fits: each point's three coefficients, and its final CIELAB difference. **/
struct Table {
    std::vector<float> coefficients;
    std::vector<float> misses;
};

};  // namespace v3d::moya::tools::rgbtable
