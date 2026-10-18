/**
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 **/

#include "Nanoseconds.h"

#include <cstdint>
#include <limits>

namespace {

/**
 * 2^64, one past the largest std::uint64_t. It is exact as a double, and every double
 * below it converts.
 **/
constexpr double UINT64_RANGE_END = 18446744073709551616.0;

};  // namespace

std::uint64_t nanoseconds(double milliseconds) {
    const double value = milliseconds * static_cast<double>(NANOSECONDS_PER_MILLISECOND);
    if (!(value >= 0.0)) {
        return 0;
    }
    if (!(value < UINT64_RANGE_END)) {
        return std::numeric_limits<std::uint64_t>::max();
    }
    return static_cast<std::uint64_t>(value);
}
