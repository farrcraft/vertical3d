/*
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 */

#include <moya/libmoya/integrate/internal/Tallies.h>

#include <moya/libmoya/device/Buffer.h>
#include <moya/libmoya/device/CpuDevice.h>
#include <moya/libmoya/device/cpu/Tally.h>
#include <moya/libmoya/integrate/Profile.h>
#include <moya/libmoya/kernel/Array.h>

#include <cstddef>
#include <cstdint>
#include <vector>

#include <glm/vec2.hpp>

namespace v3d::moya::integrate::internal {

Tallies::Tallies(device::CpuDevice & device, const glm::uvec2 & resolution) :
    device_(device), count_(static_cast<std::size_t>(resolution.x) * resolution.y) {
    const std::vector<device::cpu::Tally> zeros(count_);
    buffer_ = device_.allocate(count_ * sizeof(device::cpu::Tally));
    device_.upload(*buffer_, zeros.data(), count_ * sizeof(device::cpu::Tally));
}

kernel::Array<device::cpu::Tally> Tallies::view() const {
    return device::view<device::cpu::Tally>(*buffer_);
}

void Tallies::into(Profile & profile) const {
    std::vector<device::cpu::Tally> tallies(count_);
    device_.download(*buffer_, tallies.data(), count_ * sizeof(device::cpu::Tally));
    std::uint64_t traversing = 0;
    std::uint64_t whole = 0;
    for (const device::cpu::Tally & tally : tallies) {
        profile.nearest += tally.nearest;
        profile.any += tally.any;
        traversing += tally.traversing;
        whole += tally.whole;
    }
    profile.traversal = whole == 0 ? 0.0 : static_cast<double>(traversing) / static_cast<double>(whole);
}

};  // namespace v3d::moya::integrate::internal
