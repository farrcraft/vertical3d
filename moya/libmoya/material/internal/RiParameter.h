/*
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 */

#pragma once

#include <string>

namespace v3d::moya::material::internal {

/** A parameter of an RI shader: its name, its MaterialX type and its default. **/
struct Parameter {
    std::string name;
    std::string type;
    std::string value;
};

};  // namespace v3d::moya::material::internal
