/*
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 */

#pragma once

#include <string>

/**
 * The arguments of the RI requests that take several values of one type, one struct per
 * request. Each value is named at the call, so values of the same type cannot be passed in the
 * wrong order:
 *
 *     handler->clipping({.hither = 0.1f, .yon = 100.0f});
 **/
namespace v3d::render::offline::rib::requests {

/**
 * RiDeclare: a name and the declaration it is given, such as "uniform point".
 **/
struct Declare {
    std::string name;
    std::string type;
};

};  // namespace v3d::render::offline::rib::requests
