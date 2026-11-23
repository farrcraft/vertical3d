/*
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 */

#include <moya/libmoya/device/Buffer.h>
#include <moya/libmoya/device/CpuDevice.h>
#include <moya/libmoya/kernel/Array.h>
#include <moya/libmoya/kernel/Launch.h>

#include <atomic>
#include <cstddef>
#include <cstdint>
#include <functional>
#include <stdexcept>
#include <utility>
#include <vector>

#include <boost/shared_ptr.hpp>
#include <boost/test/unit_test.hpp>

namespace {

typedef v3d::moya::device::CpuDevice CpuDevice;

/** A count of how often each pixel ran, one per pixel in rows. **/
struct Counts {
    v3d::moya::kernel::Array<std::uint32_t> counts;
    std::uint32_t width;
};

/** A hash of each pixel's own coordinates, written to that pixel alone. **/
struct Hashes {
    v3d::moya::kernel::Array<std::uint32_t> hashes;
    std::uint32_t width;
};

/**
 * A buffer one byte past the start of an aligned one, as a device that broke the alignment would
 * give.
 **/
class Misaligned final : public v3d::moya::device::Buffer {
 public:
    explicit Misaligned(boost::shared_ptr<v3d::moya::device::Buffer> aligned) : aligned_(std::move(aligned)) {
    }

    std::size_t bytes() const override {
        return aligned_->bytes() - 1;
    }

    void * address() const override {
        return static_cast<std::byte *>(aligned_->address()) + 1;
    }

 private:
    boost::shared_ptr<v3d::moya::device::Buffer> aligned_;
};

void count(const Counts & params, v3d::moya::kernel::Item item) {
    params.counts[(item.y * params.width) + item.x] += 1;
}

void hash(const Hashes & params, v3d::moya::kernel::Item item) {
    std::uint32_t mixed = (item.x * 0x9E3779B1U) ^ (item.y + 0x7F4A7C15U);
    mixed ^= mixed >> 16;
    mixed *= 0x85EBCA6BU;
    mixed ^= mixed >> 13;
    params.hashes[(item.y * params.width) + item.x] = mixed;
}

/** How often a launch over an image of the given size runs each of its pixels. **/
std::vector<std::uint32_t> counted(CpuDevice & device, std::uint32_t width, std::uint32_t height) {
    const std::vector<std::uint32_t> zeros(static_cast<std::size_t>(width) * height, 0U);
    auto buffer = device.allocate(zeros.size() * sizeof(std::uint32_t));
    device.upload(*buffer, zeros.data(), zeros.size() * sizeof(std::uint32_t));
    device.launch(width, height, Counts { v3d::moya::device::view<std::uint32_t>(*buffer), width }, &count);
    std::vector<std::uint32_t> out(zeros.size());
    device.download(*buffer, out.data(), out.size() * sizeof(std::uint32_t));
    return out;
}

/** The hash image of the given size, launched on the device. **/
std::vector<std::uint32_t> hashed(CpuDevice & device, std::uint32_t width, std::uint32_t height) {
    auto buffer = device.allocate(static_cast<std::size_t>(width) * height * sizeof(std::uint32_t));
    device.launch(width, height, Hashes { v3d::moya::device::view<std::uint32_t>(*buffer), width }, &hash);
    std::vector<std::uint32_t> out(static_cast<std::size_t>(width) * height);
    device.download(*buffer, out.data(), out.size() * sizeof(std::uint32_t));
    return out;
}

// an image whose sides are not multiples of a tile has partial tiles at its right and bottom
BOOST_AUTO_TEST_CASE(moya_device_a_launch_runs_every_pixel_once_test) {
    CpuDevice device;
    for (const auto & [width, height] : std::vector<std::pair<std::uint32_t, std::uint32_t>> { { 67, 45 }, { 1, 1 }, { 32, 32 }, { 33, 1 } }) {
        const auto counts = counted(device, width, height);
        BOOST_TEST_CONTEXT(width << " by " << height) {
            for (const std::uint32_t runs : counts) {
                BOOST_TEST(runs == 1U);
            }
        }
    }
}

BOOST_AUTO_TEST_CASE(moya_device_an_image_with_no_pixels_runs_nothing_test) {
    CpuDevice device;
    auto buffer = device.allocate(sizeof(std::uint32_t));
    const std::uint32_t zero = 0;
    device.upload(*buffer, &zero, sizeof(zero));
    const Counts params { v3d::moya::device::view<std::uint32_t>(*buffer), 0 };
    device.launch(0, 5, params, &count);
    device.launch(5, 0, params, &count);
    std::uint32_t out = 1;
    device.download(*buffer, &out, sizeof(out));
    BOOST_TEST(out == 0U);
}

/**
 * How often a launch over an image of the given size runs each of its pixels when it may stop,
 * with whether every pixel ran.
 **/
std::pair<std::vector<std::uint32_t>, bool> stoppable(CpuDevice & device, std::uint32_t width, std::uint32_t height, const std::function<bool()> & stop) {
    const std::vector<std::uint32_t> zeros(static_cast<std::size_t>(width) * height, 0U);
    auto buffer = device.allocate(zeros.size() * sizeof(std::uint32_t));
    device.upload(*buffer, zeros.data(), zeros.size() * sizeof(std::uint32_t));
    const bool ran = device.launch(width, height, Counts { v3d::moya::device::view<std::uint32_t>(*buffer), width }, &count, stop);
    std::vector<std::uint32_t> out(zeros.size());
    device.download(*buffer, out.data(), out.size() * sizeof(std::uint32_t));
    return { out, ran };
}

// a launch whose stop never answers true runs every pixel once, and says so
BOOST_AUTO_TEST_CASE(moya_device_a_launch_that_is_not_stopped_runs_every_pixel_test) {
    CpuDevice device;
    const auto [counts, ran] = stoppable(device, 67, 45, [] { return false; });
    BOOST_TEST(ran);
    for (const std::uint32_t runs : counts) {
        BOOST_TEST(runs == 1U);
    }
}

// a launch stopped at its start runs no tile
BOOST_AUTO_TEST_CASE(moya_device_a_launch_stopped_at_its_start_runs_no_tile_test) {
    CpuDevice device;
    const auto [counts, ran] = stoppable(device, 67, 45, [] { return true; });
    BOOST_TEST(!ran);
    for (const std::uint32_t runs : counts) {
        BOOST_TEST(runs == 0U);
    }
}

// a launch on one thread stopped at its third tile runs the two before it whole, and no pixel of
// any other
BOOST_AUTO_TEST_CASE(moya_device_a_launch_stops_between_tiles_test) {
    CpuDevice device(1);
    std::atomic<std::uint32_t> asked { 0 };
    const auto [counts, ran] = stoppable(device, 96, 64, [&] { return ++asked == 3U; });
    BOOST_TEST(!ran);
    std::size_t runs = 0;
    for (const std::uint32_t each : counts) {
        BOOST_TEST(each <= 1U);
        runs += each;
    }
    BOOST_TEST(runs == 2U * CpuDevice::kTile * CpuDevice::kTile);
    // each tile ran whole or not at all
    for (std::uint32_t tile = 0; tile < 6U; ++tile) {
        const std::uint32_t left = (tile % 3U) * CpuDevice::kTile;
        const std::uint32_t top = (tile / 3U) * CpuDevice::kTile;
        const std::uint32_t first = counts[(static_cast<std::size_t>(top) * 96U) + left];
        for (std::uint32_t y = top; y < top + CpuDevice::kTile; ++y) {
            for (std::uint32_t x = left; x < left + CpuDevice::kTile; ++x) {
                BOOST_TEST(counts[(static_cast<std::size_t>(y) * 96U) + x] == first);
            }
        }
    }
}

BOOST_AUTO_TEST_CASE(moya_device_one_thread_and_every_thread_give_one_picture_test) {
    CpuDevice one(1);
    CpuDevice every;
    BOOST_TEST(one.threads() == 1U);
    const auto alone = hashed(one, 131, 77);
    const auto together = hashed(every, 131, 77);
    BOOST_TEST(alone == together, boost::test_tools::per_element());
}

BOOST_AUTO_TEST_CASE(moya_device_a_buffer_returns_what_was_copied_in_test) {
    CpuDevice device;
    const std::vector<float> in { 1.0F, -2.5F, 3.25F };
    auto buffer = device.allocate(in.size() * sizeof(float));
    BOOST_TEST(buffer->bytes() == in.size() * sizeof(float));
    device.upload(*buffer, in.data(), in.size() * sizeof(float));
    std::vector<float> out(in.size());
    device.download(*buffer, out.data(), out.size() * sizeof(float));
    BOOST_TEST(out == in, boost::test_tools::per_element());
    BOOST_TEST(v3d::moya::device::view<float>(*buffer).size == 3U);
}

BOOST_AUTO_TEST_CASE(moya_device_a_copy_larger_than_its_buffer_is_refused_test) {
    CpuDevice device;
    auto buffer = device.allocate(4);
    const std::vector<std::uint8_t> bytes(8, 0);
    std::vector<std::uint8_t> out(8, 0);
    BOOST_CHECK_THROW(device.upload(*buffer, bytes.data(), bytes.size()), std::out_of_range);
    BOOST_CHECK_THROW(device.download(*buffer, out.data(), out.size()), std::out_of_range);
}

// every buffer, of whatever size, starts at an address aligned to a buffer's alignment, so a view
// of a hierarchy's 32-byte aligned nodes is never misaligned
BOOST_AUTO_TEST_CASE(moya_device_every_buffer_is_aligned_for_kernel_code_test) {
    CpuDevice device;
    std::vector<boost::shared_ptr<v3d::moya::device::Buffer>> buffers;
    std::size_t misaligned = 0;
    for (std::size_t bytes = 1; bytes <= 256; ++bytes) {
        buffers.push_back(device.allocate(bytes));
        if (reinterpret_cast<std::uintptr_t>(buffers.back()->address()) % v3d::moya::device::Buffer::kAlignment != 0) {
            ++misaligned;
        }
    }
    BOOST_TEST(misaligned == 0U);
}

/**
 * A view of a buffer whose address is not aligned for kernel code is refused, rather than handing
 * kernel code a misaligned array.
 *
 * Covers the base: fcbca238 added the check to device::view(). Checked by removing its throw,
 * which fails this case.
 **/
BOOST_AUTO_TEST_CASE(moya_device_a_view_of_a_misaligned_buffer_is_refused_test) {
    CpuDevice device;
    const Misaligned buffer(device.allocate((sizeof(float) * 4) + 1));
    BOOST_CHECK_THROW(v3d::moya::device::view<float>(buffer), std::invalid_argument);
}

};  // namespace
