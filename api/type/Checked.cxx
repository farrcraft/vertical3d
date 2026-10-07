/**
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 **/

#include "Checked.h"

namespace v3d::type {

std::optional<uint32_t> toCount(float value, uint32_t minimum, uint32_t maximum) noexcept {
    // the bounds are compared as doubles, which hold every uint32_t exactly. A float bound
    // would round 2^32 - 1 up to 2^32, and let 2^32 through to an undefined conversion.
    const double given = value;
    // a NaN compares false both ways, so it is not inside
    const bool inside = given >= static_cast<double>(minimum) && given <= static_cast<double>(maximum);
    if (!inside) {
        return std::nullopt;
    }
    return static_cast<uint32_t>(value);
}

std::optional<uint32_t> toCount(float value, uint32_t maximum) noexcept {
    return toCount(value, 0, maximum);
}

std::optional<int32_t> toInteger(float value, int32_t minimum, int32_t maximum) noexcept {
    const double given = value;
    const bool inside = given >= static_cast<double>(minimum) && given <= static_cast<double>(maximum);
    if (!inside) {
        return std::nullopt;
    }
    return static_cast<int32_t>(value);
}

};  // namespace v3d::type
