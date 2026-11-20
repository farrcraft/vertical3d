/*
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 */

#pragma once

#include <moya/libmoya/kernel/Qualifiers.h>

#include <cstdint>

namespace v3d::moya::kernel {

/**
 * A run of values in device memory, which kernel code reads and writes by index. It owns
 * nothing: the device layer allocates the memory, and keeps it alive while a launch reads it.
 *
 * An index is not checked. Kernel code keeps every index below size.
 **/
template <typename T>
struct Array {
    T * data { nullptr };
    std::uint32_t size { 0 };

    MOYA_KERNEL T & operator[](std::uint32_t index) const {
        return data[index];
    }
};

};  // namespace v3d::moya::kernel
