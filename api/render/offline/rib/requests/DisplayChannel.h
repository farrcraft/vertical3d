/*
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 */

#pragma once

#include <string>

namespace v3d::render::offline::rib::requests {

/**
 * RenderMan's DisplayChannel: a channel a Display's mode can name, declared as a type and a name,
 * such as "varying color diffuse". Its parameters say what it collects.
 **/
struct DisplayChannel {
    std::string declaration;
};

};  // namespace v3d::render::offline::rib::requests
