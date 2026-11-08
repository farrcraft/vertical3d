/*
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 */

#pragma once

#include <cstdint>
#include <limits>

#include <glm/mat4x4.hpp>

namespace v3d::moya::device::cpu {

/** The first triangle of an instance that places a group, which holds none. **/
inline constexpr std::uint32_t kGroup = std::numeric_limits<std::uint32_t>::max();

/**
 * An instance as the CPU traverses it. A leaf of a group's hierarchy holds instances, and a ray
 * that reaches one is moved into its prototype's space and traverses the prototype's hierarchy
 * from its root.
 *
 * An instance of a mesh names the mesh's first triangle, and the leaves of the mesh's hierarchy
 * hold triangles from there. An instance of a group names kGroup, and the leaves of the group's
 * hierarchy hold instances.
 **/
struct Instance {
    /** From the space of the group that holds the instance into its prototype's. **/
    glm::mat4 toPrototype { 1.0F };

    std::uint32_t root { 0 };
    std::uint32_t first { kGroup };

    /** The instance's index in the scene, which a hit's chain names it by. **/
    std::uint32_t id { 0 };

    /** The kinds of ray the instance is visible to. **/
    std::uint32_t visibility { 0 };
};

};  // namespace v3d::moya::device::cpu
