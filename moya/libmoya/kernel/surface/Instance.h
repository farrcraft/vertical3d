/*
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 */

#pragma once

#include <cstdint>
#include <limits>

#include <glm/mat4x4.hpp>

namespace v3d::moya::kernel::surface {

/** The geometry of an instance that places a group, which places no geometry. **/
inline constexpr std::uint32_t kGroup = std::numeric_limits<std::uint32_t>::max();

/**
 * An instance as shading reads it: its transformation from its prototype's space into the space
 * of the group that holds it, its keys when it moves, and the geometry it places.
 **/
struct Instance {
    /** The transformation when it holds still, and at its first sample when it moves. **/
    glm::mat4 transform { 1.0F };

    /** Its first key and how many it has, which is more than one when it moves. **/
    std::uint32_t keys { 0 };
    std::uint32_t keyCount { 0 };

    std::uint32_t geometry { kGroup };
};

};  // namespace v3d::moya::kernel::surface
