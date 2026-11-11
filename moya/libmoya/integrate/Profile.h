/*
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 */

#pragma once

#include <cstdint>

namespace v3d::moya::integrate {

/**
 * What a render measures of itself, for a benchmark. The time is always measured. The rays and
 * the share of time in traversal are counted only when counted is true. A counted render runs an
 * entry point that times each ray it traces, so it is slower, and its time is not the render's
 * own.
 **/
struct Profile {
    /** Whether to count the rays and the time in traversal. **/
    bool counted { false };

    /** The seconds the passes took, summed, without the noise checks and checkpoints between. **/
    double seconds { 0.0 };

    /** The rays traced for their nearest hit, and for whether they meet anything. **/
    std::uint64_t nearest { 0 };
    std::uint64_t any { 0 };

    /** The share of the passes' time on every thread that traversal took, from 0 to 1. **/
    double traversal { 0.0 };
};

};  // namespace v3d::moya::integrate
