/*
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 */

#pragma once

#include <moya/libmoya/kernel/Array.h>

#include <cstddef>
#include <cstdint>
#include <limits>
#include <stdexcept>

namespace v3d::moya::device {

/**
 * A block of memory on one device, which kernel code reads and writes through a kernel::Array.
 * The device that allocated it owns how it is held: host memory on the CPU, device memory on a
 * GPU. A buffer lives as long as the last reference to it, and a launch that reads it holds one.
 **/
class Buffer {
 public:
    virtual ~Buffer() = default;

    /** How many bytes it holds. **/
    virtual std::size_t bytes() const = 0;

    /**
     * Where kernel code finds it, on the device that allocated it. On a GPU the host cannot
     * read through it.
     **/
    virtual void * address() const = 0;
};

/**
 * The buffer as an array of T, holding as many whole values as fit in it.
 *
 * @throws std::out_of_range when it holds more values than a kernel index reaches
 **/
template <typename T>
kernel::Array<T> view(const Buffer & buffer) {
    const std::size_t count = buffer.bytes() / sizeof(T);
    if (count > std::numeric_limits<std::uint32_t>::max()) {
        throw std::out_of_range("a buffer larger than a kernel index reaches");
    }
    return kernel::Array<T> { static_cast<T *>(buffer.address()), static_cast<std::uint32_t>(count) };
}

};  // namespace v3d::moya::device
