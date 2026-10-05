/**
 * Vertical3D
 * Copyright(c) 2023 Joshua Farr(josh@farrcraft.com)
 **/

#pragma once

#include <api/type/Flags.h>

#include <cstdint>

namespace v3d::input {
enum class DeviceType : uint32_t {
    Mouse = (1 << 0),
    Keyboard = (1 << 1)
};

using DeviceTypes = v3d::type::Flags<DeviceType>;

constexpr DeviceTypes operator|(DeviceType lhs, DeviceType rhs) {
    return DeviceTypes(lhs) | rhs;
}
};  // namespace v3d::input
