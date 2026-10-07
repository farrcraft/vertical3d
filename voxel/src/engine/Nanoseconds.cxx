/**
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 **/

#include "Nanoseconds.h"

#include <limits>

std::uint64_t nanoseconds(double milliseconds) {
    const double value = milliseconds * 1.0e6;
    if (!(value >= 0.0)) {
        return 0;
    }
    // 2^64 is exact as a double, and every double below it converts
    if (!(value < 18446744073709551616.0)) {
        return std::numeric_limits<std::uint64_t>::max();
    }
    return static_cast<std::uint64_t>(value);
}
