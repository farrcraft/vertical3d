/**
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 **/

#pragma once

#include <unicode/utypes.h>

namespace v3d::font::internal {

/**
 * @return whether an ICU call failed. A warning is not a failure
 **/
inline bool failed(UErrorCode status) {
    return U_FAILURE(status) != 0;
}

};  // namespace v3d::font::internal
