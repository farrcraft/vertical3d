/*
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 */

#pragma once

#include <cstdint>
#include <vector>

namespace v3d::moya::geometry {

/** The instances a group holds, each by its index among every instance. **/
struct Group {
    std::vector<std::uint32_t> instances;
};

};  // namespace v3d::moya::geometry
