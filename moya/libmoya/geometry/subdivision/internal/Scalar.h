/*
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 */

#pragma once

namespace v3d::moya::geometry::subdivision::internal {

/**
 * One float of a primvar, as OpenSubdiv's refinement and its patches interpolate a value. Its two
 * members are the names OpenSubdiv's templates call.
 **/
struct Scalar {
    float value { 0.0F };

    // NOLINTNEXTLINE(readability-identifier-naming) - the name OpenSubdiv calls
    void Clear() {
        value = 0.0F;
    }

    // NOLINTNEXTLINE(readability-identifier-naming) - the name OpenSubdiv calls
    void AddWithWeight(const Scalar & source, float weight) {
        value += weight * source.value;
    }
};

};  // namespace v3d::moya::geometry::subdivision::internal
