/**
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 **/

#pragma once

#include <api/render/realtime/vulkan/device/Device.h>

#include <vulkan/vulkan.h>

#include <cstdint>
#include <string>
#include <string_view>
#include <vector>

#include <boost/shared_ptr.hpp>

namespace v3d::render::realtime::vulkan::frame {

/**
 * How long the device spent on each pass of a frame, from timestamps written around it.
 *
 * Each frame in flight has its own query pool. A slot's timestamps are read when the slot is
 * begun again, after its fence has signalled. The numbers are framesInFlight frames old, and
 * reading them never waits on the device.
 *
 * On a device whose graphics queue writes no timestamps, Timings is disabled and every call
 * does nothing.
 **/
class Timings final {
 public:
    /**
     * How many spans a frame can time. Each pass is one span. Spans past this many are not
     * timed.
     **/
    static constexpr uint32_t capacity = 64;

    /**
     * One span: a pass, or whatever a caller recording its own commands named.
     **/
    struct Timing final {
        std::string name;
        double milliseconds { 0.0 };
    };

    Timings(const boost::shared_ptr<device::Device>& device, uint32_t framesInFlight);
    ~Timings();

    Timings(const Timings&) = delete;
    Timings& operator=(const Timings&) = delete;

    /**
     * @return whether the device writes timestamps
     **/
    bool enabled() const noexcept;

    /**
     * Read what a slot recorded the last time it was used, then reset its queries in the
     * command buffer it is beginning. Ring::begin() calls it once the slot's fence has
     * signalled.
     *
     * @param submitted false when the slot's last use was abandoned before its submit, so it
     *        recorded nothing that can be read
     **/
    void begin(VkCommandBuffer commands, uint32_t slot, bool submitted = true);

    /**
     * Write a timestamp before a span and after it. Spans do not nest.
     **/
    void open(VkCommandBuffer commands, std::string_view name);
    void close(VkCommandBuffer commands);

    /**
     * @return the spans of the most recent frame read back, in the order they were recorded
     **/
    const std::vector<Timing>& last() const noexcept;

 private:
    struct Slot final {
        VkQueryPool pool { VK_NULL_HANDLE };
        std::vector<std::string> names;
        bool open { false };
    };

    void read(Slot* slot);

    boost::shared_ptr<device::Device> device_;
    std::vector<Slot> slots_;
    uint32_t current_;
    uint64_t mask_;
    double period_;
    std::vector<Timing> last_;
};

};  // namespace v3d::render::realtime::vulkan::frame
