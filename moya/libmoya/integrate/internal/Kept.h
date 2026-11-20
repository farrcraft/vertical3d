/*
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 */

#pragma once

#include <moya/libmoya/integrate/internal/Coverage.h>
#include <moya/libmoya/integrate/internal/Deep.h>
#include <moya/libmoya/kernel/integrate/Covered.h>

#include <chrono>
#include <cstdint>
#include <vector>

namespace v3d::moya::integrate::internal {

/**
 * What a render keeps on the host of what its samples cover: coverage and deep lists, and the
 * seconds adding each has taken.
 **/
struct Kept {
    Coverage coverage;
    Deep deep;
    double coverageSeconds { 0.0 };
    double deepSeconds { 0.0 };

    /** Adds a pass's records of samples to a pixel to both, timing each. **/
    void add(const std::vector<kernel::integrate::Covered> & records, std::uint32_t samples) {
        const std::chrono::steady_clock::time_point start = std::chrono::steady_clock::now();
        coverage.add(records, samples);
        const std::chrono::steady_clock::time_point covered = std::chrono::steady_clock::now();
        deep.add(records, samples);
        coverageSeconds += std::chrono::duration<double>(covered - start).count();
        deepSeconds += std::chrono::duration<double>(std::chrono::steady_clock::now() - covered).count();
    }
};

};  // namespace v3d::moya::integrate::internal
