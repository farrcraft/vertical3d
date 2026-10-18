/**
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 **/

#include <api/image/Image.h>
#include <api/render/realtime/vulkan/frame/Capture.h>

#include <cstddef>
#include <cstdint>
#include <vector>

#include <boost/test/unit_test.hpp>

using v3d::image::Image;
using v3d::render::realtime::vulkan::frame::Capture;

namespace {

/**
 * @return one texel, in the channel order a copy from a swapchain image leaves it
 **/
std::vector<unsigned char> texel(unsigned char first, unsigned char second, unsigned char third, unsigned char fourth) {
    return std::vector<unsigned char>{first, second, third, fourth};
}

BOOST_AUTO_TEST_SUITE(capture_convert_test)

/**
 * A BGRA swapchain is the common case, and the one a naive copy gets wrong. Red and blue come
 * back in the opposite order to the one a PNG stores.
 **/
BOOST_AUTO_TEST_CASE(a_bgra_chain_is_swizzled) {
    const std::vector<unsigned char> pixels = texel(10, 20, 30, 40);

    const auto img = Capture::convert(pixels.data(), 1, 1, VK_FORMAT_B8G8R8A8_UNORM);

    BOOST_CHECK_EQUAL(static_cast<int>((*img)[0]), 30);
    BOOST_CHECK_EQUAL(static_cast<int>((*img)[1]), 20);
    BOOST_CHECK_EQUAL(static_cast<int>((*img)[2]), 10);
}

/**
 * An SRGB swapchain, which an app can request, has the same channel order as its UNORM twin.
 * The transfer function does not change which byte is red.
 **/
BOOST_AUTO_TEST_CASE(an_srgb_chain_is_swizzled_the_same_way) {
    const std::vector<unsigned char> pixels = texel(10, 20, 30, 40);

    const auto img = Capture::convert(pixels.data(), 1, 1, VK_FORMAT_B8G8R8A8_SRGB);

    BOOST_CHECK_EQUAL(static_cast<int>((*img)[0]), 30);
    BOOST_CHECK_EQUAL(static_cast<int>((*img)[2]), 10);
}

/**
 * An RGBA swapchain image is copied through untouched.
 **/
BOOST_AUTO_TEST_CASE(an_rgba_chain_keeps_its_order) {
    const std::vector<unsigned char> pixels = texel(10, 20, 30, 40);

    const auto img = Capture::convert(pixels.data(), 1, 1, VK_FORMAT_R8G8B8A8_UNORM);

    BOOST_CHECK_EQUAL(static_cast<int>((*img)[0]), 10);
    BOOST_CHECK_EQUAL(static_cast<int>((*img)[1]), 20);
    BOOST_CHECK_EQUAL(static_cast<int>((*img)[2]), 30);
}

/**
 * Whatever the swapchain image holds in its alpha channel, a captured frame is opaque. The image
 * has already been composited, and a viewer that read the alpha would show whatever is behind
 * the file through it.
 **/
BOOST_AUTO_TEST_CASE(alpha_is_written_opaque) {
    const std::vector<unsigned char> pixels = texel(10, 20, 30, 0);

    const auto img = Capture::convert(pixels.data(), 1, 1, VK_FORMAT_B8G8R8A8_UNORM);

    BOOST_CHECK_EQUAL(static_cast<int>((*img)[3]), 255);
}

/**
 * Every texel is converted, and the image is the size of the swapchain image.
 **/
BOOST_AUTO_TEST_CASE(the_whole_image_is_converted) {
    constexpr std::size_t width = 2;
    constexpr std::size_t height = 3;

    std::vector<unsigned char> pixels(width * height * 4);
    for (std::size_t i = 0; i < pixels.size(); i++) {
        pixels[i] = static_cast<unsigned char>(i);
    }

    const auto img = Capture::convert(pixels.data(), static_cast<uint32_t>(width),
        static_cast<uint32_t>(height), VK_FORMAT_B8G8R8A8_UNORM);

    BOOST_CHECK_EQUAL(img->width(), width);
    BOOST_CHECK_EQUAL(img->height(), height);
    BOOST_CHECK(img->format() == Image::Format::RGBA);

    // the last texel of six, which a loop that stopped one short would leave zeroed
    BOOST_CHECK_EQUAL(static_cast<int>((*img)[20]), 22);
    BOOST_CHECK_EQUAL(static_cast<int>((*img)[22]), 20);
}

BOOST_AUTO_TEST_SUITE_END()

};  // namespace
