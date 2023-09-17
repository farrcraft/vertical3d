/**
 * Vertical3D
 * Copyright(c) 2023 Joshua Farr(josh@farrcraft.com)
 **/

#pragma once

#include <cstdint>

namespace v3d::input {
    enum class DeviceType : uint32_t {
        Mouse = (1 << 0),
        Keyboard = (1 << 1)
    };

    constexpr DeviceType operator|(DeviceType lhs, DeviceType rhs) {
        return static_cast<DeviceType>(static_cast<int>(lhs) | static_cast<int>(rhs));
    }

    constexpr bool operator&(int lhs, DeviceType rhs) {
        return lhs & static_cast<int>(rhs);
    }

    constexpr int& operator|=(int& lhs, DeviceType rhs) {  // NOLINT(runtime/references)
        lhs = lhs | static_cast<int>(rhs);
        return lhs;
    }
};  // namespace v3d::input
