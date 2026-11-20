/*
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 */

#include <moya/libmoya/integrate/internal/Records.h>

#include <moya/libmoya/device/Buffer.h>
#include <moya/libmoya/device/CpuDevice.h>
#include <moya/libmoya/kernel/Array.h>
#include <moya/libmoya/kernel/integrate/Covered.h>

#include <cstddef>
#include <cstdint>
#include <vector>

#include <glm/vec2.hpp>

namespace v3d::moya::integrate::internal {

Records::Records(device::CpuDevice & device, const glm::uvec2 & resolution, std::uint32_t samples, bool wanted) :
    device_(device), pixels_(static_cast<std::size_t>(resolution.x) * resolution.y) {
    if (!wanted) {
        return;
    }
    const std::size_t records = pixels_ * samples;
    const std::vector<kernel::integrate::Covered> empty(records);
    buffer_ = device_.allocate(records * sizeof(kernel::integrate::Covered));
    device_.upload(*buffer_, empty.data(), records * sizeof(kernel::integrate::Covered));
}

kernel::Array<kernel::integrate::Covered> Records::view() const {
    return buffer_ ? device::view<kernel::integrate::Covered>(*buffer_) : kernel::Array<kernel::integrate::Covered> {};
}

std::vector<kernel::integrate::Covered> Records::taken(std::uint32_t samples) const {
    if (!buffer_) {
        return {};
    }
    std::vector<kernel::integrate::Covered> out(pixels_ * samples);
    device_.download(*buffer_, out.data(), out.size() * sizeof(kernel::integrate::Covered));
    return out;
}

};  // namespace v3d::moya::integrate::internal
