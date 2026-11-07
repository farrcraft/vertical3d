/*
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 */

#pragma once

#include <moya/libmoya/device/Buffer.h>

#include <cstddef>
#include <vector>

namespace v3d::moya::device::internal {

/** Host memory, as the CPU device's buffer. **/
class HostBuffer final : public Buffer {
 public:
    explicit HostBuffer(std::size_t bytes) : memory_(bytes) {
    }

    std::size_t bytes() const override {
        return memory_.size();
    }

    void * address() const override {
        return memory_.data();
    }

 private:
    // kernel code writes through a buffer that host code holds as const
    mutable std::vector<std::byte> memory_;
};

};  // namespace v3d::moya::device::internal
