/*
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 */

#pragma once

#include <tbb/task_arena.h>

#include <algorithm>
#include <limits>

namespace v3d::moya::device::internal {

/**
 * The TBB arena a CPU device's launches run in. It is behind its own header so that only the
 * device's source includes TBB.
 **/
class Arena final {
 public:
    /** @param threads the arena's concurrency, or 0 for TBB's own default **/
    explicit Arena(unsigned int threads)
        : arena_(threads == 0 ? tbb::task_arena::automatic : concurrency(threads)) {
    }

    tbb::task_arena & get() {
        return arena_;
    }

 private:
    /** A thread count as TBB takes it, which is an int. **/
    static int concurrency(unsigned int threads) {
        return static_cast<int>(std::min<unsigned int>(threads, std::numeric_limits<int>::max()));
    }

    tbb::task_arena arena_;
};

};  // namespace v3d::moya::device::internal
