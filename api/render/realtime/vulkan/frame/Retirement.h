/**
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 **/

#pragma once

#include <cstddef>
#include <cstdint>
#include <deque>
#include <functional>

namespace v3d::render::realtime::vulkan::frame {

/**
 * Destruction held back until every frame that might still read what is being destroyed has
 * finished.
 *
 * An entry is a callback, so this does not depend on what it destroys. That may be a texture,
 * a descriptor set going back to a free list, or anything later that has to outlive the frames
 * in flight. It needs no device either, because the count of frames begun is handed in by
 * whoever is counting - the ring.
 **/
class Retirement final {
 public:
    /**
     * @param framesInFlight how many frames may be recorded ahead of the device
     **/
    explicit Retirement(uint32_t framesInFlight);

    /**
     * Destroy everything still held, without waiting. The caller makes sure the device is
     * idle first.
     **/
    ~Retirement();

    Retirement(const Retirement&) = delete;
    Retirement& operator=(const Retirement&) = delete;

    /**
     * Hold a destruction back.
     * @param begun how many frames had begun when the thing was released
     * @param destroy what to run once those frames have finished
     **/
    void retire(uint64_t begun, std::function<void()> destroy);

    /**
     * Run every destruction whose frames have finished.
     *
     * A frame that had begun when something was released has finished by the time framesInFlight
     * more frames have begun, because beginning a frame waits on the fence of the one that used
     * its slot last.
     *
     * @param begun how many frames have begun, counting the one just begun
     **/
    void collect(uint64_t begun);

    /**
     * Run every destruction still held, finished or not.
     **/
    void flush();

    /**
     * @return how many destructions are still held
     **/
    std::size_t pending() const noexcept;

 private:
    struct Entry {
        uint64_t begun;
        std::function<void()> destroy;
    };

    uint32_t framesInFlight_;
    std::deque<Entry> entries_;
};

};  // namespace v3d::render::realtime::vulkan::frame
