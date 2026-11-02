/**
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 **/

#pragma once

#include <string>

namespace v3d::font::face {

/**
 * A value on one of a variable font's axes, such as { "wght", 650 } for a weight between
 * regular and bold or { "wdth", 75 } for a condensed width.
 *
 * The tag is the axis's four letter name, and the value is in the axis's own units, as the
 * font's named instances give them. A value outside the axis's range is taken as its nearest
 * end. A face without the axis ignores it.
 **/
struct Variation final {
    std::string tag;
    float value = 0.0F;

    bool operator==(const Variation& other) const = default;
};

};  // namespace v3d::font::face
