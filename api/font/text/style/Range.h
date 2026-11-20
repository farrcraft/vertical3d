/*
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 */

#pragma once

#include <cstddef>

#include "Style.h"

namespace v3d::font::text::style {

/**
 * A style over the bytes from one offset to another of a text.
 **/
struct Range final {
    std::size_t start = 0;
    std::size_t end = 0;
    Style style;
};

};  // namespace v3d::font::text::style
