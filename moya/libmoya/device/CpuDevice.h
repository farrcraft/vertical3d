/*
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 */

#pragma once

#include <moya/libmoya/device/Buffer.h>
#include <moya/libmoya/device/Device.h>
#include <moya/libmoya/kernel/Launch.h>

#include <cstddef>
#include <cstdint>
#include <functional>
#include <string>

#include <boost/shared_ptr.hpp>

namespace v3d::moya::device {

namespace internal {
class Arena;
};  // namespace internal

/**
 * The CPU as a device. Its memory is host memory, and a launch runs a kernel function over the
 * tiles of an image on oneTBB's threads.
 *
 * A launch splits the image into tiles of 32 by 32 pixels, smaller at the right and bottom edges,
 * and runs each tile's pixels one after another on one thread. Tiles run in any order, so a
 * kernel function writes only its own pixel's results, and a picture does not depend on the
 * order tiles finish in or on how many threads run them.
 **/
class CpuDevice final : public Device {
 public:
    /** The side of a tile, in pixels. **/
    static constexpr std::uint32_t kTile = 32;

    /**
     * @param threads how many threads a launch runs on at most, or 0 for as many as TBB gives
     *        this process. A host's own limit on TBB applies as well.
     **/
    explicit CpuDevice(unsigned int threads = 0);

    std::string name() const override;
    boost::shared_ptr<Buffer> allocate(std::size_t bytes) override;
    void upload(Buffer & to, const void * from, std::size_t bytes) override;
    void download(const Buffer & from, void * to, std::size_t bytes) override;

    /** How many threads a launch runs on at most. **/
    unsigned int threads() const;

    /**
     * Runs host work on the device's threads: the task once for each index below count, in any
     * order and on any thread, and returns when every one has run. A task may call each() again,
     * and its work joins the same threads. Each task writes only what is its own index's, so the
     * result does not depend on the order tasks finish in.
     **/
    void each(std::size_t count, const std::function<void(std::size_t)> & task);

    /**
     * Runs a kernel function once for each pixel of an image of the given size, and returns when
     * every pixel has run. An image with no pixels runs nothing.
     **/
    template <typename Params>
    void launch(std::uint32_t width, std::uint32_t height, const Params & params, kernel::Function<Params> function) {
        const Bound<Params> bound { &params, function };
        run(width, height, &Bound<Params>::call, &bound);
    }

 private:
    /** A kernel function and its parameters, behind one pointer that run() passes to each item. **/
    template <typename Params>
    struct Bound {
        const Params * params;
        kernel::Function<Params> function;

        static void call(const void * bound, kernel::Item item) {
            const auto * self = static_cast<const Bound *>(bound);
            self->function(*self->params, item);
        }
    };

    /** Runs call for each pixel, over tiles on the arena's threads. **/
    void run(std::uint32_t width, std::uint32_t height, void (*call)(const void *, kernel::Item), const void * bound);

    boost::shared_ptr<internal::Arena> arena_;
};

};  // namespace v3d::moya::device
