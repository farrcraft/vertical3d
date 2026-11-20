/*
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 */

#pragma once

#include <cstddef>
#include <string>

namespace v3d::script {

/**
 * Where a script could not be stripped, and why.
 **/
struct Problem final {
    std::size_t line = 0;       /**< from 1 **/
    std::size_t column = 0;     /**< from 1, in bytes **/
    std::string message;
};

};  // namespace v3d::script
