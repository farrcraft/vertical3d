/*
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 */

#include <moya/libmoya/scene/Handle.h>

namespace v3d::moya::scene {

bool Handle::valid() const noexcept {
    return generation != 0;
}

};  // namespace v3d::moya::scene
