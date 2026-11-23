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
    /** The alignment every buffer's address has, enough for any type kernel code reads. **/
    static constexpr std::size_t kAlignment = 64;

    virtual ~Buffer() = default;

    /** How many bytes it holds. **/
    virtual std::size_t bytes() const = 0;

    /**
     * Where kernel code finds it, on the device that allocated it, aligned to kAlignment. On a
     * GPU the host cannot read through it.
     **/
    virtual void * address() const = 0;
};

/**
 * The buffer as an array of T, holding as many whole values as fit in it. T's alignment is at
 * most a buffer's.
 *
 * @throws std::out_of_range when it holds more values than a kernel index reaches
 * @throws std::invalid_argument when its address is not aligned to kAlignment
 **/
template <typename T>
kernel::Array<T> view(const Buffer & buffer) {
    static_assert(alignof(T) <= Buffer::kAlignment, "a type more aligned than a buffer");
    const std::size_t count = buffer.bytes() / sizeof(T);
    if (count > std::numeric_limits<std::uint32_t>::max()) {
        throw std::out_of_range("a buffer larger than a kernel index reaches");
    }
    if (reinterpret_cast<std::uintptr_t>(buffer.address()) % Buffer::kAlignment != 0) {
        throw std::invalid_argument("a buffer whose address is not aligned for kernel code");
    }
    return kernel::Array<T> { static_cast<T *>(buffer.address()), static_cast<std::uint32_t>(count) };
}

};  // namespace v3d::moya::device
