/**
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 **/

#pragma once

#include <cstdint>

/**
 * A span the device timed, in milliseconds, as whole nanoseconds, with the fraction dropped.
 * A negative span or a NaN is zero. A span of 2^64 nanoseconds or longer, +infinity among them,
 * is the largest count.
 **/
std::uint64_t nanoseconds(double milliseconds);
