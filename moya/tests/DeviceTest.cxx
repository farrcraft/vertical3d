/*
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 */

#include <moya/libmoya/device/Buffer.h>
#include <moya/libmoya/device/CpuDevice.h>
#include <moya/libmoya/kernel/Array.h>
#include <moya/libmoya/kernel/Launch.h>

#include <cstdint>
#include <cstddef>
#include <stdexcept>
#include <utility>
#include <vector>

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

};  // namespace
