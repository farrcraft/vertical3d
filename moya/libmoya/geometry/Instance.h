/*
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 */

#pragma once

#include <moya/libmoya/kernel/geometry/Key.h>
#include <moya/libmoya/kernel/geometry/Motion.h>
#include <moya/libmoya/kernel/surface/Material.h>

#include <cstdint>
#include <vector>

#include <glm/mat4x4.hpp>

namespace v3d::moya::geometry {

/**
 * What an instance places: a geometry, a group of instances, or a volume. No group holds an
 * instance of a volume among the instances its hierarchy is built over, since a ray is tracked
 * through a volume rather than traced to it.
 **/
enum class Prototype : std::uint8_t {
    Geometry,
    Group,
    Volume
};

/**
 * An instance that places a prototype. Its transformation takes the prototype's space to the
 * space of the group that holds the instance, and its visibility holds the kinds of ray it is
 * visible to.
 **/
struct Instance {
    /** The instance's index in the scene, which a hit's chain names it by. **/
    std::uint32_t id { 0 };

    Prototype kind { Prototype::Geometry };

    /** The prototype's index among the geometries or the groups. **/
    std::uint32_t prototype { 0 };

    /** The transformation at its first sample, which holds at every time when it has no keys. **/
    glm::mat4 transform { 1.0F };

    /** The transformation at each of its samples, when it has more than one. **/
    std::vector<kernel::geometry::Key> keys;

    std::uint32_t visibility { 0 };

    /** Whether a material that emits from the geometry it places emits from both faces. **/
    bool doubleSided { false };

    /** Its material among the prototypes' materials, or kernel::surface::kNoMaterial. **/
    std::uint32_t material { kernel::surface::kNoMaterial };

    /** Its priority where the media of closed surfaces overlap. **/
    std::int32_t priority { 0 };

    /** Whether the instance's transformation moves over the shutter. **/
    bool moves() const {
        return keys.size() > 1;
    }
};

};  // namespace v3d::moya::geometry
