/**
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 **/

#include <api/image/Image.h>
#include <api/render/realtime/DeviceContext.h>
#include <api/render/realtime/Frame.h>
#include <api/render/realtime/Handle.h>
#include <api/render/realtime/Pass.h>
#include <api/render/realtime/RenderTarget.h>
#include <api/render/realtime/Textures.h>
#include <api/render/realtime/canvas/Canvas.h>
#include <api/render/realtime/vulkan/frame/Capture.h>
#include <api/render/realtime/vulkan/memory/TextureFactory.h>

#include <vulkan/vulkan.h>

#include <array>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <string>
#include <vector>

#include <boost/make_shared.hpp>
#include <boost/shared_ptr.hpp>
#include <boost/test/unit_test.hpp>
#include <glm/vec2.hpp>
#include <glm/vec4.hpp>

#include "Headless.h"

namespace {

using v3d::render::realtime::TextureHandle;
using v3d::render::realtime::canvas::Canvas;
using v3d::render::realtime::vulkan::memory::TextureFactory;

const VkFormat kColourFormat = VK_FORMAT_R8G8B8A8_UNORM;
const uint32_t kSize = 32;

/**
 * A square from 0.2 to 0.8 of an em, as four lines wound counter clockwise. It is laid out as
 * the curve shader reads it: each line a curve with its control on its end, two texels a curve.
 **/
constexpr std::array<float, 32> kCurves{
    0.2F, 0.2F, 0.8F, 0.2F, 0.8F, 0.2F, 0.0F, 0.0F,  // along the bottom
    0.8F, 0.2F, 0.8F, 0.8F, 0.8F, 0.8F, 0.0F, 0.0F,  // up the right
    0.8F, 0.8F, 0.2F, 0.8F, 0.2F, 0.8F, 0.0F, 0.0F,  // along the top
    0.2F, 0.8F, 0.2F, 0.2F, 0.2F, 0.2F, 0.0F, 0.0F,  // down the left
};

/**
 * The square's one horizontal and one vertical band. The transform is all zeros, so every
 * sample is in band 0. The horizontal band lists the right then the left side, the vertical
 * band the top then the bottom, each from the one reaching furthest along its ray.
 **/
constexpr std::array<float, 16> kBands{
    0.0F, 0.0F, 0.0F, 0.0F,  // the band transform
    1.0F, 1.0F, 0.0F, 0.0F,  // one vertical band and one horizontal band
    2.0F, 12.0F, 2.0F, 14.0F,  // the horizontal band's count and list, then the vertical band's
    1.0F, 3.0F, 2.0F, 0.0F,  // the two lists
};

/**
 * Draw the square over the whole target, turned about its centre by an angle, and read the
 * target back.
 **/
boost::shared_ptr<v3d::image::Image> draw(v3d::test::Headless* headless, float turn, const std::string& path) {
    const boost::shared_ptr<v3d::render::realtime::Textures> textures = headless->context->textures();
    const TextureHandle curves = textures->texture(TextureFactory::Texels{ .data = kCurves.data(), .width = 8, .height = 1 });
    const TextureHandle bands = textures->texture(TextureFactory::Texels{ .data = kBands.data(), .width = 4, .height = 1 });

    Canvas canvas;
    canvas.resize(glm::uvec2(kSize, kSize));
    canvas.clear();
    const float half = static_cast<float>(kSize) / 2.0F;
    // each corner turned about the target's centre. em y runs upwards and the canvas downwards
    const auto corner = [&](float x, float y, const glm::vec2& em) {
        const glm::vec2 from = glm::vec2(x, y) * half;
        const glm::vec2 turned((from.x * std::cos(turn)) - (from.y * std::sin(turn)), (from.x * std::sin(turn)) + (from.y * std::cos(turn)));
        return Canvas::Vertex{ .position = glm::vec2(half) + turned, .uv = em, .colour = glm::vec4(1.0F) };
    };
    const std::vector<Canvas::Vertex> corners{ corner(-1.0F, -1.0F, glm::vec2(0.0F, 1.0F)), corner(1.0F, -1.0F, glm::vec2(1.0F, 1.0F)),
        corner(1.0F, 1.0F, glm::vec2(1.0F, 0.0F)), corner(-1.0F, 1.0F, glm::vec2(0.0F, 0.0F)) };
    canvas.curves(corners, std::vector<uint32_t>{ 0, 1, 2, 2, 3, 0 }, curves, bands);

    const boost::shared_ptr<v3d::render::realtime::RenderTarget> target = boost::make_shared<v3d::render::realtime::RenderTarget>(*headless->context,
        v3d::render::realtime::RenderTarget::Spec{ .width = kSize, .height = kSize, .colour = kColourFormat });
    v3d::render::realtime::Frame frame;
    const boost::shared_ptr<v3d::render::realtime::Pass> pass = frame.pass("colour");
    pass->target(target);
    pass->clearColour(glm::vec4(0.0F, 0.0F, 0.0F, 1.0F));
    headless->context->quads()->submit(canvas, pass.get());
    return headless->picture(frame, *target, path);
}

/**
 * @return the red of a texel of a picture
 **/
int red(v3d::image::Image& picture, uint32_t x, uint32_t y) {
    return picture.data()[((static_cast<std::size_t>(y) * kSize) + x) * 4];
}

BOOST_AUTO_TEST_SUITE(curve_text_test)

/**
 * A square of curves covers the pixels inside it wholly and those well outside it not at all, and
 * its edge pixels are partly covered.
 **/
BOOST_AUTO_TEST_CASE(curves_cover_what_they_enclose) {
    v3d::test::Headless headless(kColourFormat, kSize, kSize);
    const boost::shared_ptr<v3d::image::Image> picture = draw(&headless, 0.0F, "data_out/curve_square.png");
    BOOST_REQUIRE(picture);
    BOOST_CHECK(headless.silent());
    // the square runs from 6.4 to 25.6 pixels on each axis
    BOOST_CHECK_EQUAL(red(*picture, 16, 16), 255);
    BOOST_CHECK_EQUAL(red(*picture, 8, 8), 255);
    BOOST_CHECK_EQUAL(red(*picture, 24, 24), 255);
    BOOST_CHECK_EQUAL(red(*picture, 2, 16), 0);
    BOOST_CHECK_EQUAL(red(*picture, 16, 29), 0);
    BOOST_CHECK(red(*picture, 6, 16) > 0 && red(*picture, 6, 16) < 255);
}

/**
 * The same square turned an eighth of a turn still covers its middle and leaves the corners of
 * the target, which it no longer reaches, empty.
 **/
BOOST_AUTO_TEST_CASE(curves_stay_whole_when_turned) {
    v3d::test::Headless headless(kColourFormat, kSize, kSize);
    const boost::shared_ptr<v3d::image::Image> picture = draw(&headless, 0.7853982F, "data_out/curve_square_turned.png");
    BOOST_REQUIRE(picture);
    BOOST_CHECK(headless.silent());
    BOOST_CHECK_EQUAL(red(*picture, 16, 16), 255);
    // the turned square reaches 13.6 pixels from the centre along each axis
    BOOST_CHECK_EQUAL(red(*picture, 16, 5), 255);
    BOOST_CHECK_EQUAL(red(*picture, 4, 4), 0);
    BOOST_CHECK_EQUAL(red(*picture, 27, 27), 0);
}

BOOST_AUTO_TEST_SUITE_END()

};  // namespace
