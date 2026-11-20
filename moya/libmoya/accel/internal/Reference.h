/*
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 */

#pragma once

#include <moya/libmoya/accel/Box.h>

#include <cstdint>

namespace v3d::moya::accel::internal {

/** A primitive, or the part of one a spatial split kept, as the builder sorts it. **/
struct Reference {
    Box box;
    std::uint32_t primitive { 0 };
};

};  // namespace v3d::moya::accel::internal
