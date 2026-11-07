/*
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 */

#include <moya/libmoya/device/CpuDevice.h>

#include <moya/libmoya/device/Buffer.h>
#include <moya/libmoya/device/internal/Arena.h>
#include <moya/libmoya/device/internal/HostBuffer.h>
#include <moya/libmoya/kernel/Launch.h>
#include <tbb/blocked_range2d.h>
#include <tbb/parallel_for.h>
#include <tbb/partitioner.h>

#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <cstring>
#include <stdexcept>
#include <string>

#include <boost/make_shared.hpp>

namespace v3d::moya::device {

namespace {

void check(const Buffer & buffer, std::size_t bytes) {
    if (bytes > buffer.bytes()) {
        throw std::out_of_range("a copy larger than its buffer");
    }
}

};  // namespace

CpuDevice::CpuDevice(unsigned int threads) : arena_(boost::make_shared<internal::Arena>(threads)) {
}

std::string CpuDevice::name() const {
    return "CPU";
}

boost::shared_ptr<Buffer> CpuDevice::allocate(std::size_t bytes) {
    return boost::make_shared<internal::HostBuffer>(bytes);
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

unsigned int CpuDevice::threads() const {
    return static_cast<unsigned int>(arena_->get().max_concurrency());
}

void CpuDevice::run(std::uint32_t width, std::uint32_t height, void (*call)(const void *, kernel::Item), const void * bound) {
    if (width == 0 || height == 0) {
        return;
    }
    // the range is over tiles, not pixels, and a grain of one with the simple partitioner gives
    // each task one tile, so a tile's pixels never split between threads
    const std::uint32_t rows = (height + kTile - 1) / kTile;
    const std::uint32_t columns = (width + kTile - 1) / kTile;
    const tbb::blocked_range2d<std::uint32_t> tiles(0, rows, 1, 0, columns, 1);
    arena_->get().execute([&] {
        tbb::parallel_for(
            tiles,
            [&](const tbb::blocked_range2d<std::uint32_t> & range) {
                for (std::uint32_t row = range.rows().begin(); row != range.rows().end(); ++row) {
                    for (std::uint32_t column = range.cols().begin(); column != range.cols().end(); ++column) {
                        const std::uint32_t top = row * kTile;
                        const std::uint32_t left = column * kTile;
                        const std::uint32_t bottom = std::min(top + kTile, height);
                        const std::uint32_t right = std::min(left + kTile, width);
                        for (std::uint32_t y = top; y < bottom; ++y) {
                            for (std::uint32_t x = left; x < right; ++x) {
                                call(bound, kernel::Item { x, y });
                            }
                        }
                    }
                }
            },
            tbb::simple_partitioner());
    });
}

};  // namespace v3d::moya::device
