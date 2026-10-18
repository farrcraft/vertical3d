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
 * Destructions delayed until every frame that might still read the destroyed object has
 * finished.
 *
 * Each entry is a callback, so Retirement does not depend on what it destroys. That may be a
 * texture, a descriptor set returned to a free list, or anything else that has to outlive the
 * frames in flight. Retirement needs no device either. Ring passes in the count of frames
 * begun.
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
     * Delay a destruction.
     * @param last the last frame that may name the released object, as a count of frames
     *        begun
     * @param destroy what to run once that frame has finished
     **/
    void retire(uint64_t last, std::function<void()> destroy);

    /**
     * Run every destruction whose frames have finished.
     *
     * An entry's last frame has finished by the time framesInFlight more frames have begun,
     * because beginning a frame waits on the fence of the one that used its slot last.
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
        uint64_t last;
        std::function<void()> destroy;
    };

    uint32_t framesInFlight_;
    std::deque<Entry> entries_;
};

};  // namespace v3d::render::realtime::vulkan::frame
