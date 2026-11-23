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
    /**
     * Whether to count the rays, and the time in traversal, programs, texture lookups, tracking,
     * grid lookups and the hair model.
     **/
    bool counted { false };

    /** The seconds the passes took, summed, without the noise checks and checkpoints between. **/
    double seconds { 0.0 };

    /** The rays traced for their nearest hit, and for whether they meet anything. **/
    std::uint64_t nearest { 0 };
    std::uint64_t any { 0 };

    /** The share of the passes' time on every thread that traversal took, from 0 to 1. **/
    double traversal { 0.0 };

    /**
     * The shares of the passes' time on every thread that materials' programs and texture
     * lookups took, from 0 to 1. A program's share holds the lookups it makes.
     **/
    double programs { 0.0 };
    double lookups { 0.0 };

    /**
     * The shares of the passes' time on every thread that tracking through media, the volumes'
     * grid lookups and the hair model took, from 0 to 1. Tracking holds a surface's shadow rays
     * in a scene of media, and the traversal, grid lookups, programs and light samples it makes.
     * The hair model's share is the time a hair's BSDF is made, sampled and evaluated.
     **/
    double tracking { 0.0 };
    double grids { 0.0 };
    double hair { 0.0 };

    /**
     * The seconds the host took after each pass, summed, to add its samples' Cryptomatte coverage
     * and to join their deep lists. Each is part of seconds.
     **/
    double coverage { 0.0 };
    double deep { 0.0 };
};

};  // namespace v3d::moya::integrate
