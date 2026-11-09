/*
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 */

#pragma once

#include "Value.h"

namespace v3d::render::offline::sl::runtime {

/**
 * The cone a surface's illuminance loop gathers light from at each point: its axis and its half
 * angle in radians. Both null for a loop with no cone, which gathers from every direction. A
 * light with a `solar` angle chooses its direction against it.
 **/
class Cone final {
 public:
    const Value* axis = nullptr;
    const Value* angle = nullptr;
};

};  // namespace v3d::render::offline::sl::runtime
