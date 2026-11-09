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
    /** The id of the world, which no object of the scene has. **/
    static constexpr std::uint32_t kWorld = 0xffffffffU;

    /** The group's index in the scene, or kWorld for the world. **/
    std::uint32_t id { kWorld };

    std::vector<std::uint32_t> instances;
};

};  // namespace v3d::moya::geometry
