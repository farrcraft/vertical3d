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
 * @return one texel, in the order a copy off the chain leaves it
 **/
std::vector<unsigned char> texel(unsigned char a, unsigned char b, unsigned char c, unsigned char d) {
    return std::vector<unsigned char>{a, b, c, d};
}

};  // namespace

BOOST_AUTO_TEST_SUITE(capture_convert_test)

/**
 * A bgra chain is the common case, and the one a naive copy gets wrong: red and blue come
 * back the other way round from the order a png wants them in.
 **/
BOOST_AUTO_TEST_CASE(a_bgra_chain_is_swizzled) {
    const std::vector<unsigned char> pixels = texel(10, 20, 30, 40);

    const auto img = Capture::convert(pixels.data(), 1, 1, VK_FORMAT_B8G8R8A8_UNORM);

    BOOST_CHECK_EQUAL(static_cast<int>((*img)[0]), 30);
    BOOST_CHECK_EQUAL(static_cast<int>((*img)[1]), 20);
    BOOST_CHECK_EQUAL(static_cast<int>((*img)[2]), 10);
}

/**
 * The srgb chain a consumer asks for under ADR-0049 is the same channel order as its unorm
 * neighbour - the transfer function is not what decides which byte is red.
 **/
BOOST_AUTO_TEST_CASE(an_srgb_chain_is_swizzled_the_same_way) {
    const std::vector<unsigned char> pixels = texel(10, 20, 30, 40);

    const auto img = Capture::convert(pixels.data(), 1, 1, VK_FORMAT_B8G8R8A8_SRGB);

    BOOST_CHECK_EQUAL(static_cast<int>((*img)[0]), 30);
    BOOST_CHECK_EQUAL(static_cast<int>((*img)[2]), 10);
}

/**
 * An rgba chain is copied through untouched.
 **/
BOOST_AUTO_TEST_CASE(an_rgba_chain_keeps_its_order) {
    const std::vector<unsigned char> pixels = texel(10, 20, 30, 40);

    const auto img = Capture::convert(pixels.data(), 1, 1, VK_FORMAT_R8G8B8A8_UNORM);

    BOOST_CHECK_EQUAL(static_cast<int>((*img)[0]), 10);
    BOOST_CHECK_EQUAL(static_cast<int>((*img)[1]), 20);
    BOOST_CHECK_EQUAL(static_cast<int>((*img)[2]), 30);
}

/**
 * Whatever the chain presented in its alpha channel, a captured frame is opaque: the image
 * has already been composited, so a viewer reading that alpha would show the file through
 * whatever is behind it.
 **/
BOOST_AUTO_TEST_CASE(alpha_is_written_opaque) {
    const std::vector<unsigned char> pixels = texel(10, 20, 30, 0);

    const auto img = Capture::convert(pixels.data(), 1, 1, VK_FORMAT_B8G8R8A8_UNORM);

    BOOST_CHECK_EQUAL(static_cast<int>((*img)[3]), 255);
}

/**
 * Every texel is converted, and the image is the size the chain was.
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
