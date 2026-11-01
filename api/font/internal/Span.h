/**
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 **/

#pragma once

#include <cstddef>

namespace v3d::font::internal {

/**
 * The bytes from one offset to another of a text, such as a line before it is placed.
 **/
struct Span {
    std::size_t start = 0;
    std::size_t end = 0;
};

};  // namespace v3d::font::internal
