/*
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 */

#include <moya/libmoya/integrate/internal/TrailSteps.h>

#include <moya/libmoya/device/Buffer.h>
#include <moya/libmoya/device/CpuDevice.h>
#include <moya/libmoya/integrate/Trails.h>
#include <moya/libmoya/kernel/Array.h>
#include <moya/libmoya/kernel/integrate/Trail.h>

#include <cstddef>
#include <string>
#include <vector>

#include <glm/vec2.hpp>

namespace v3d::moya::integrate::internal {

TrailSteps::TrailSteps(device::CpuDevice & device, const glm::uvec2 & resolution, bool recording) :
    device_(device), count_(recording ? std::size_t { resolution.x } * resolution.y * kernel::integrate::kTrailSteps : 0U) {
    if (count_ == 0U) {
        return;
    }
    const std::vector<kernel::integrate::Step> empty(count_);
    buffer_ = device_.allocate(count_ * sizeof(kernel::integrate::Step));
    device_.upload(*buffer_, empty.data(), count_ * sizeof(kernel::integrate::Step));
}

kernel::Array<kernel::integrate::Step> TrailSteps::view() const {
    return buffer_ ? device::view<kernel::integrate::Step>(*buffer_) : kernel::Array<kernel::integrate::Step> {};
}

void TrailSteps::into(Trails & trails, const std::vector<std::string> & labels) const {
    std::vector<kernel::integrate::Step> steps(count_);
    if (count_ != 0U) {
        device_.download(*buffer_, steps.data(), count_ * sizeof(kernel::integrate::Step));
    }
    trails.pixels.assign(count_ / kernel::integrate::kTrailSteps, {});
    for (std::size_t i = 0; i < count_; ++i) {
        if (steps[i].used != 0U) {
            trails.pixels[i / kernel::integrate::kTrailSteps].push_back(steps[i]);
        }
    }
    trails.labels = labels;
}

};  // namespace v3d::moya::integrate::internal
