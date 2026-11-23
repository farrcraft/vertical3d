/*
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 */

#pragma once

#include <string>

namespace v3d::moya::material::internal {

/**
 * An output of an RI shader's translation: the OpenPBR input it feeds, its type, and the node that
 * gives it.
 **/
struct Feed {
    std::string target;
    std::string type;
    std::string node;
};

};  // namespace v3d::moya::material::internal
