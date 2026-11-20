/*
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 */

#pragma once

#include <moya/libmoya/device/Buffer.h>
#include <moya/libmoya/device/CpuDevice.h>
#include <moya/libmoya/kernel/Array.h>
#include <moya/libmoya/kernel/integrate/Covered.h>

#include <cstddef>
#include <cstdint>
#include <vector>

#include <boost/shared_ptr.hpp>

#include <glm/vec2.hpp>

namespace v3d::moya::integrate::internal {

/**
 * The buffer each camera sample of a pass records what it covers in, which Cryptomatte and deep
 * output read after the pass: up to a pass's samples for each pixel, in the device's memory. A
 * render that needs neither holds none. The device outlives it.
 **/
class Records final {
 public:
    /** A buffer for passes of up to samples to a pixel when wanted is true, and none otherwise. **/
    Records(device::CpuDevice & device, const glm::uvec2 & resolution, std::uint32_t samples, bool wanted);

    /** The buffer as the kernel takes it, which is empty while the render records nothing. **/
    kernel::Array<kernel::integrate::Covered> view() const;

    /** The records of a pass of samples to a pixel, each pixel's in sample order, or none. **/
    std::vector<kernel::integrate::Covered> taken(std::uint32_t samples) const;

 private:
    device::CpuDevice & device_;
    std::size_t pixels_ { 0 };
    boost::shared_ptr<device::Buffer> buffer_;
};

};  // namespace v3d::moya::integrate::internal
