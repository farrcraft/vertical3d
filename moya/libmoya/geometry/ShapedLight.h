/*
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 */

#pragma once

#include <moya/libmoya/scene/Handle.h>
#include <moya/libmoya/scene/Scene.h>

#include <cstdint>

namespace v3d::moya::geometry {

/** A light with a shape, and the instance in the world that places its shape. **/
struct ShapedLight {
    scene::Handle handle;

    /** Its instance's index among the instances. **/
    std::uint32_t instance { 0 };
};

/**
 * Whether a light of the scene has a shape that the world places: a rect, disk or cylinder light,
 * or a sphere light of a radius above zero that is not treated as a point.
 **/
bool shaped(const scene::Scene & scene, const scene::Handle & handle);

};  // namespace v3d::moya::geometry
