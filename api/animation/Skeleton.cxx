/**
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 **/

#include "Skeleton.h"

namespace v3d::animation {

bool Skeleton::empty() const noexcept {
    return joints.empty();
}

};  // namespace v3d::animation
