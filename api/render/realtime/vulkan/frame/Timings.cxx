/**
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 **/

#include "Timings.h"

#include <api/render/realtime/vulkan/device/Result.h>

#include <vector>

namespace v3d::render::realtime::vulkan::frame {

/**
 **/
Timings::Timings(const boost::shared_ptr<device::Device>& device, uint32_t framesInFlight) :
    device_(device),
    current_(0),
    mask_(0),
    period_(0.0) {
    const uint32_t bits = device_->families().timestampBits;
    if (bits == 0) {
        return;
    }
    mask_ = bits >= 64 ? ~0ULL : (1ULL << bits) - 1ULL;
    period_ = static_cast<double>(device_->timestampPeriod());

    slots_.resize(framesInFlight);
    for (Slot& slot : slots_) {
        VkQueryPoolCreateInfo info{};
        info.sType = VK_STRUCTURE_TYPE_QUERY_POOL_CREATE_INFO;
        info.queryType = VK_QUERY_TYPE_TIMESTAMP;
        info.queryCount = capacity * 2;
        const VkResult result = vkCreateQueryPool(device_->handle(), &info, nullptr, &slot.pool);
        device::check(result, "Unable to create a vulkan timestamp pool");
    }
}

/**
 **/
Timings::~Timings() {
    for (Slot& slot : slots_) {
        if (slot.pool != VK_NULL_HANDLE) {
            vkDestroyQueryPool(device_->handle(), slot.pool, nullptr);
        }
    }
}

/**
 **/
bool Timings::enabled() const noexcept {
    return !slots_.empty();
}

/**
 **/
void Timings::begin(VkCommandBuffer commands, uint32_t slot, bool submitted) {
    if (!enabled()) {
        return;
    }
    current_ = slot % static_cast<uint32_t>(slots_.size());
    Slot& active = slots_[current_];
    // what an abandoned frame recorded was never submitted, so there is nothing to read
    if (submitted) {
        read(&active);
    }
    active.names.clear();
    active.open = false;
    // reset in the command buffer rather than on the host, which needs a feature the device
    // is not asked for
    vkCmdResetQueryPool(commands, active.pool, 0, capacity * 2);
}

/**
 **/
void Timings::read(Slot* slot) {
    if (slot->names.empty()) {
        return;
    }
    const auto count = static_cast<uint32_t>(slot->names.size() * 2);
    std::vector<uint64_t> stamps(count, 0);
    // no wait: the slot's fence has signalled, so every query it wrote is available
    const VkResult result = vkGetQueryPoolResults(device_->handle(), slot->pool, 0, count,
        stamps.size() * sizeof(uint64_t), stamps.data(), sizeof(uint64_t), VK_QUERY_RESULT_64_BIT);
    if (result != VK_SUCCESS) {
        return;
    }
    last_.clear();
    last_.reserve(slot->names.size());
    for (std::size_t index = 0; index < slot->names.size(); ++index) {
        // masked, because a queue writing fewer than 64 bits wraps, and a span across the
        // wrap is still the difference of the two
        const uint64_t ticks = (stamps[index * 2 + 1] - stamps[index * 2]) & mask_;
        last_.push_back(Timing{ slot->names[index], static_cast<double>(ticks) * period_ / 1.0e6 });
    }
}

/**
 **/
void Timings::open(VkCommandBuffer commands, std::string_view name) {
    if (!enabled()) {
        return;
    }
    Slot& slot = slots_[current_];
    if (slot.open || slot.names.size() >= capacity) {
        return;
    }
    const auto query = static_cast<uint32_t>(slot.names.size() * 2);
    slot.names.emplace_back(name);
    slot.open = true;
    vkCmdWriteTimestamp2(commands, VK_PIPELINE_STAGE_2_ALL_COMMANDS_BIT, slot.pool, query);
}

/**
 **/
void Timings::close(VkCommandBuffer commands) {
    if (!enabled()) {
        return;
    }
    Slot& slot = slots_[current_];
    if (!slot.open) {
        return;
    }
    slot.open = false;
    const auto query = static_cast<uint32_t>(slot.names.size() * 2 - 1);
    vkCmdWriteTimestamp2(commands, VK_PIPELINE_STAGE_2_ALL_COMMANDS_BIT, slot.pool, query);
}

/**
 **/
const std::vector<Timings::Timing>& Timings::last() const noexcept {
    return last_;
}

};  // namespace v3d::render::realtime::vulkan::frame
