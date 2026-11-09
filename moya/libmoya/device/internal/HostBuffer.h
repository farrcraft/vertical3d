/*
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 */

#pragma once

#include <moya/libmoya/device/Buffer.h>

#include <cstddef>
#include <cstring>
#include <new>

namespace v3d::moya::device::internal {

/**
 * Host memory, as the CPU device's buffer. It starts zeroed, at an address aligned to
 * kAlignment.
 **/
class HostBuffer final : public Buffer {
 public:
    explicit HostBuffer(std::size_t bytes) : bytes_(bytes), memory_(static_cast<std::byte *>(::operator new(bytes, std::align_val_t { kAlignment }))) {
        std::memset(memory_, 0, bytes_);
    }

    HostBuffer(const HostBuffer &) = delete;
    HostBuffer & operator=(const HostBuffer &) = delete;

    ~HostBuffer() override {
        ::operator delete(memory_, std::align_val_t { kAlignment });
    }

    std::size_t bytes() const override {
        return bytes_;
    }

    void * address() const override {
        return memory_;
    }

 private:
    std::size_t bytes_;
    // kernel code writes through a buffer that host code holds as const
    std::byte * memory_;
};

};  // namespace v3d::moya::device::internal
