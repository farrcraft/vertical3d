/*
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 */

#pragma once

#include <cstdint>
#include <limits>

#include <glm/mat4x4.hpp>

namespace v3d::moya::device::cpu {

/** The geometry of an instance that places a group, which places no geometry. **/
inline constexpr std::uint32_t kGroup = std::numeric_limits<std::uint32_t>::max();

/**
 * Whether the surfaces an instance places are cut out. An instance that names no material leaves
 * them as the chain around it has them, and one that names a material makes them its material's.
 **/
enum class Cut : std::uint8_t {
    Inherited,
    Solid,
    Out
};

/**
 * An instance as the CPU traverses it. A leaf of a group's hierarchy holds instances, and a ray
 * that reaches one is moved into its prototype's space and traverses the prototype's hierarchy
 * from its root.
 *
 * An instance of a geometry names the geometry, and the leaves of the geometry's hierarchy hold
 * the indices of its primitives. An instance of a group names kGroup, and the leaves of the
 * group's hierarchy hold instances.
 **/
struct Instance {
    /** From the space of the group that holds the instance into its prototype's, when still. **/
    glm::mat4 toPrototype { 1.0F };

    /** The instance's first key and how many it has, which is more than one when it moves. **/
    std::uint32_t keys { 0 };
    std::uint32_t keyCount { 0 };

    std::uint32_t root { 0 };

    /** One when the prototype's hierarchy is of moving nodes. **/
    std::uint32_t moving { 0 };

    std::uint32_t geometry { kGroup };

    /** The instance's index in the scene, which a hit's chain names it by. **/
    std::uint32_t id { 0 };

    /** The kinds of ray the instance is visible to. **/
    std::uint32_t visibility { 0 };

    Cut cut { Cut::Inherited };
};

};  // namespace v3d::moya::device::cpu
