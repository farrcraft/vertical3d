/*
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 */

#pragma once

#include <cstdint>

namespace v3d::moya::device::cpu {

/**
 * What a profiled render counts for one pixel: the rays it traced for their nearest hit and for
 * whether they meet anything, the time its traversal took, and the time the whole pixel took.
 * Each time is in the processor's time-stamp ticks, so only a ratio of two of them means
 * anything.
 **/
struct Tally {
    std::uint64_t nearest { 0 };
    std::uint64_t any { 0 };
    std::uint64_t traversing { 0 };
    std::uint64_t whole { 0 };
};

};  // namespace v3d::moya::device::cpu
