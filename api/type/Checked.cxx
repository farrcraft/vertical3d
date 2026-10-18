/**
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 **/

#include "Checked.h"

#include <cstdint>
#include <optional>

namespace v3d::type {

std::optional<uint32_t> toCount(float value, const Range<uint32_t>& range) noexcept {
    // the bounds are compared as doubles, which hold every uint32_t exactly. A float bound
    // would round 2^32 - 1 up to 2^32, and let 2^32 through to an undefined conversion.
    const double given = value;
    // a NaN compares false both ways, so it is not inside
    const bool inside = given >= static_cast<double>(range.minimum) && given <= static_cast<double>(range.maximum);
    if (!inside) {
        return std::nullopt;
    }
    return static_cast<uint32_t>(value);
}

std::optional<uint32_t> toCount(float value, uint32_t maximum) noexcept {
    return toCount(value, {.minimum = 0, .maximum = maximum});
}

std::optional<int32_t> toInteger(float value, const Range<int32_t>& range) noexcept {
    const double given = value;
    const bool inside = given >= static_cast<double>(range.minimum) && given <= static_cast<double>(range.maximum);
    if (!inside) {
        return std::nullopt;
    }
    return static_cast<int32_t>(value);
}

};  // namespace v3d::type
