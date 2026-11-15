/*
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 */

#pragma once

#include <moya/libmoya/integrate/internal/Coverage.h>
#include <moya/libmoya/integrate/internal/Deep.h>

namespace v3d::moya::integrate::internal {

/** What a render keeps on the host of what its samples cover: coverage and deep lists. **/
struct Kept {
    Coverage coverage;
    Deep deep;
};

};  // namespace v3d::moya::integrate::internal
