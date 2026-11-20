/*
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 */

#pragma once

#include <moya/libmoya/device/Buffer.h>
#include <moya/libmoya/device/CpuDevice.h>
#include <moya/libmoya/integrate/Trails.h>
#include <moya/libmoya/kernel/Array.h>
#include <moya/libmoya/kernel/integrate/Trail.h>

#include <cstddef>
#include <string>
#include <vector>

#include <boost/shared_ptr.hpp>

#include <glm/vec2.hpp>

namespace v3d::moya::integrate::internal {

/**
 * The steps a render records each pixel's events in, kernel::integrate::kTrailSteps to a pixel,
 * row by row, in the device's memory, each empty to begin. A render that records nothing holds
 * none. The device outlives it.
 **/
class TrailSteps final {
 public:
    /** Steps for every pixel when recording is true, and none otherwise. **/
    TrailSteps(device::CpuDevice & device, const glm::uvec2 & resolution, bool recording);

    /** The steps as the kernel takes them, which are empty while it records nothing. **/
    kernel::Array<kernel::integrate::Step> view() const;

    /** Each pixel's recorded events, in order, and the light groups' names, given to trails. **/
    void into(Trails & trails, const std::vector<std::string> & labels) const;

 private:
    device::CpuDevice & device_;
    std::size_t count_;
    boost::shared_ptr<device::Buffer> buffer_;
};

};  // namespace v3d::moya::integrate::internal
