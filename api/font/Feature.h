/**
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 **/

#pragma once

#include <cstdint>
#include <string>

namespace v3d::font {

/**
 * An OpenType feature turned on or off for a whole string, such as { "tnum" } for tabular
 * figures or { "kern", 0 } for no kerning.
 *
 * The tag is the feature's four letter name. The value is 0 to turn it off and 1 to turn it on.
 * A feature that chooses among alternates, such as `salt`, takes the alternate's number.
 **/
struct Feature final {
    std::string tag;
    std::uint32_t value = 1;

    bool operator==(const Feature& other) const = default;
};

};  // namespace v3d::font
