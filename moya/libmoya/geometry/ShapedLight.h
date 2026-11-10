/*
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 */

#pragma once

#include <moya/libmoya/scene/Handle.h>

#include <cstdint>

namespace v3d::moya::geometry {

/** A light with a shape, and the instance in the world that places its shape. **/
struct ShapedLight {
    scene::Handle handle;

    /** Its instance's index among the instances. **/
    std::uint32_t instance { 0 };
};

};  // namespace v3d::moya::geometry
