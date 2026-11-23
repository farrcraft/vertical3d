/*
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 */

#include <moya/libmoya/device/CpuDevice.h>

#include <moya/libmoya/device/Buffer.h>
#include <moya/libmoya/device/cpu/Isa.h>
#include <moya/libmoya/device/CpuLibrary.h>
#include <moya/libmoya/device/cpu/Module.h>
#include <moya/libmoya/device/internal/Arena.h>
#include <moya/libmoya/device/internal/HostBuffer.h>
#include <moya/libmoya/kernel/Launch.h>
#include <tbb/blocked_range.h>
#include <tbb/blocked_range2d.h>
#include <tbb/parallel_for.h>
#include <tbb/partitioner.h>

#include <algorithm>
#include <atomic>
#include <cstddef>
#include <cstdint>
#include <cstring>
#include <functional>
#include <mutex>
#include <stdexcept>
#include <string>

#include <boost/make_shared.hpp>

#include <glm/vec2.hpp>

namespace v3d::moya::device {

namespace {

/** Calls the kernel for each pixel of one tile, which the image's edges may cut short. **/
// NOLINTNEXTLINE(bugprone-easily-swappable-parameters) - the tile's row, then its column
void tile(std::uint32_t row, std::uint32_t column, glm::uvec2 size, void (*call)(const void *, kernel::Item), const void * bound) {
    const std::uint32_t top = row * CpuDevice::kTile;
    const std::uint32_t left = column * CpuDevice::kTile;
    const std::uint32_t bottom = std::min(top + CpuDevice::kTile, size.y);
    const std::uint32_t right = std::min(left + CpuDevice::kTile, size.x);
    for (std::uint32_t y = top; y < bottom; ++y) {
        for (std::uint32_t x = left; x < right; ++x) {
            call(bound, kernel::Item { x, y });
        }
    }
}

};  // namespace

CpuDevice::CpuDevice(unsigned int threads, cpu::Isa newest) :
    arena_(boost::make_shared<internal::Arena>(threads)), isa_(std::min(newest, newestIsa())) {
}

std::string CpuDevice::name() const {
    return "CPU";
}

boost::shared_ptr<Buffer> CpuDevice::allocate(std::size_t bytes) {
    return boost::make_shared<internal::HostBuffer>(bytes);
}

void CpuDevice::check(const Buffer & buffer, std::size_t bytes) {
    if (bytes > buffer.bytes()) {
        throw std::out_of_range("a copy larger than its buffer");
    }
}

void CpuDevice::upload(Buffer & to, const void * from, std::size_t bytes) {
    check(to, bytes);
    if (bytes > 0) {
        std::memcpy(to.address(), from, bytes);
    }
}

void CpuDevice::download(const Buffer & from, void * to, std::size_t bytes) {
    check(from, bytes);
    if (bytes > 0) {
        std::memcpy(to, from.address(), bytes);
    }
}

cpu::Isa CpuDevice::isa() const {
    return isa_;
}

const cpu::Module & CpuDevice::module() {
    const std::lock_guard<std::mutex> lock(loading_);
    if (!library_) {
        library_ = boost::make_shared<CpuLibrary>(isa_);
    }
    return library_->module();
}

unsigned int CpuDevice::threads() const {
    return static_cast<unsigned int>(arena_->get().max_concurrency());
}

void CpuDevice::each(std::size_t count, const std::function<void(std::size_t)> & task) {
    if (count == 0) {
        return;
    }
    // a grain of one with the simple partitioner gives each task an index of its own
    arena_->get().execute([&] {
        tbb::parallel_for(
            tbb::blocked_range<std::size_t>(0, count, 1),
            [&](const tbb::blocked_range<std::size_t> & range) {
                for (std::size_t index = range.begin(); index != range.end(); ++index) {
                    task(index);
                }
            },
            tbb::simple_partitioner());
    });
}

bool CpuDevice::run(std::uint32_t width, std::uint32_t height, void (*call)(const void *, kernel::Item), const void * bound, const std::function<bool()> * stop) {
    if (width == 0 || height == 0) {
        return true;
    }
    // the range is over tiles, not pixels, and a grain of one with the simple partitioner gives
    // each task one tile, so a tile's pixels never split between threads
    const std::uint32_t rows = (height + kTile - 1) / kTile;
    const std::uint32_t columns = (width + kTile - 1) / kTile;
    const tbb::blocked_range2d<std::uint32_t> tiles(0, rows, 1, 0, columns, 1);
    // once set, every tile not yet started is passed over
    std::atomic<bool> stopped { false };
    arena_->get().execute([&] {
        tbb::parallel_for(
            tiles,
            [&](const tbb::blocked_range2d<std::uint32_t> & range) {
                for (std::uint32_t row = range.rows().begin(); row != range.rows().end(); ++row) {
                    for (std::uint32_t column = range.cols().begin(); column != range.cols().end(); ++column) {
                        if (stopped.load(std::memory_order_relaxed)) {
                            return;
                        }
                        if (stop != nullptr && (*stop)()) {
                            stopped.store(true, std::memory_order_relaxed);
                            return;
                        }
                        tile(row, column, glm::uvec2 { width, height }, call, bound);
                    }
                }
            },
            tbb::simple_partitioner());
    });
    return !stopped.load(std::memory_order_relaxed);
}

};  // namespace v3d::moya::device
