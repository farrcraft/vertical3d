/*
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 */

#pragma once

#include <cstdint>

namespace v3d::moya::device::cpu {

/**
 * What a profiled render counts for one pixel. It counts the rays traced for their nearest hit
 * and for whether they meet anything. It times the traversal, the materials' programs, the
 * texture lookups, tracking through media, the volumes' grid lookups, the hair model and the
 * whole pixel. A program's time holds the time of the lookups it makes, and tracking's the time
 * of the grid lookups and programs it makes.
 * Each time is in the processor's time-stamp ticks, so only a ratio of two of them means
 * anything.
 **/
struct Tally {
    std::uint64_t nearest { 0 };
    std::uint64_t any { 0 };
    std::uint64_t traversing { 0 };
    std::uint64_t programs { 0 };
    std::uint64_t lookups { 0 };
    std::uint64_t tracking { 0 };
    std::uint64_t grids { 0 };
    std::uint64_t hair { 0 };
    std::uint64_t whole { 0 };
};

};  // namespace v3d::moya::device::cpu
