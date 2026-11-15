/*
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 */

#pragma once

#include <cstddef>
#include <string>

namespace v3d::moya::lpe {

/** Why an expression cannot be read, and the place in it, from zero, where reading stopped. **/
struct Failure {
    std::size_t place { 0 };
    std::string reason;
};

};  // namespace v3d::moya::lpe
