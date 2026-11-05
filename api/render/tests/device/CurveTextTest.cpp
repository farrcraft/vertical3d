/**
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 **/

#include <api/image/Image.h>
#include <api/render/realtime/DeviceContext.h>
#include <api/render/realtime/frame/Frame.h>
#include <api/render/realtime/resource/Handle.h>
#include <api/render/realtime/frame/Pass.h>
#include <api/render/realtime/frame/RenderTarget.h>
#include <api/render/realtime/resource/Textures.h>
#include <api/render/realtime/canvas/Canvas.h>
#include <api/render/realtime/canvas/DepthOrder.h>
#include <api/render/realtime/canvas/World.h>
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
#include <glm/mat4x4.hpp>
#include <glm/vec2.hpp>
#include <glm/vec3.hpp>
#include <glm/vec4.hpp>

#include "Headless.h"

namespace {

using v3d::render::realtime::resource::TextureHandle;
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
 * The square's bands, then a colour program at value 16 that draws two layers inside it. The
 * first is a gradient from red at 0.2 of an em across to blue at 0.8. The second is the text's
 * colour at half its alpha, over the first.
 **/
constexpr std::array<float, 88> kProgram{
    0.0F, 0.0F, 0.0F, 0.0F, 1.0F, 1.0F, 0.0F, 0.0F, 2.0F, 12.0F, 2.0F, 14.0F, 1.0F, 3.0F, 2.0F, 0.0F,  // the square
    2.0F, 0.0F, 0.0F, 0.0F,  // two steps
    // a fill inside the square: linear, padded, two stops at 68, mapped from em units as they are
    0.0F, 0.0F, 0.0F, 1.0F, 0.0F, 0.0F, 0.0F, 0.0F, 1.0F, 0.0F, 2.0F, 68.0F,
    1.0F, 0.0F, 0.0F, 1.0F, 0.0F, 0.0F, 0.2F, 0.0F, 0.8F, 0.0F, 0.0F, 0.0F,
    // a fill inside the square: solid, its one stop at 80
    0.0F, 0.0F, 0.0F, 1.0F, 0.0F, 0.0F, 0.0F, 0.0F, 0.0F, 0.0F, 1.0F, 80.0F,
    1.0F, 0.0F, 0.0F, 1.0F, 0.0F, 0.0F, 0.0F, 0.0F, 0.0F, 0.0F, 0.0F, 0.0F,
    0.0F, 1.0F, 0.0F, 0.0F, 1.0F, 0.0F,  // red at 0
    1.0F, 0.0F, 0.0F, 1.0F, 1.0F, 0.0F,  // blue at 1
    0.0F, 0.0F, 0.0F, 0.0F, 0.5F, 1.0F,  // the text's colour at half its alpha
    0.0F, 0.0F,
};

/**
 * The value the program starts at, and the bit that names it as a program.
 **/
constexpr uint32_t kProgramStart = 16;
constexpr uint32_t kColourProgram = 0x80000000U;

/**
 * Draw the square over the whole target, turned about its centre by an angle, and read the
 * target back.
 **/
boost::shared_ptr<v3d::image::Image> draw(v3d::test::Headless* headless, float turn, const std::string& path, bool program = false) {
    const boost::shared_ptr<v3d::render::realtime::resource::Textures> textures = headless->context->textures();
    const TextureHandle curves = textures->texture(TextureFactory::Texels{ .data = kCurves.data(), .width = 8, .height = 1 });
    const TextureHandle bands = program ? textures->texture(TextureFactory::Texels{ .data = kProgram.data(), .width = 22, .height = 1 })
                                        : textures->texture(TextureFactory::Texels{ .data = kBands.data(), .width = 4, .height = 1 });
    const uint32_t glyph = program ? kProgramStart | kColourProgram : 0;
    const glm::vec4 colour = program ? glm::vec4(0.0F, 1.0F, 0.0F, 1.0F) : glm::vec4(1.0F);

    Canvas canvas;
    canvas.resize(glm::uvec2(kSize, kSize));
    canvas.clear();
    const float half = static_cast<float>(kSize) / 2.0F;
    // each corner turned about the target's centre. em y runs upwards and the canvas downwards
    const auto corner = [&](float x, float y, const glm::vec2& em) {
        const glm::vec2 from = glm::vec2(x, y) * half;
        const glm::vec2 turned((from.x * std::cos(turn)) - (from.y * std::sin(turn)), (from.x * std::sin(turn)) + (from.y * std::cos(turn)));
        return Canvas::Vertex{ .position = glm::vec2(half) + turned, .uv = em, .colour = colour, .glyph = glyph };
    };
    const std::vector<Canvas::Vertex> corners{ corner(-1.0F, -1.0F, glm::vec2(0.0F, 1.0F)), corner(1.0F, -1.0F, glm::vec2(1.0F, 1.0F)),
        corner(1.0F, 1.0F, glm::vec2(1.0F, 0.0F)), corner(-1.0F, 1.0F, glm::vec2(0.0F, 0.0F)) };
    canvas.curves(corners, std::vector<uint32_t>{ 0, 1, 2, 2, 3, 0 }, curves, bands);

    const boost::shared_ptr<v3d::render::realtime::frame::RenderTarget> target = boost::make_shared<v3d::render::realtime::frame::RenderTarget>(*headless->context,
        v3d::render::realtime::frame::RenderTarget::Spec{ .width = kSize, .height = kSize, .colour = kColourFormat });
    v3d::render::realtime::frame::Frame frame;
    const boost::shared_ptr<v3d::render::realtime::frame::Pass> pass = frame.pass("colour");
    pass->target(target);
    pass->clearColour(glm::vec4(0.0F, 0.0F, 0.0F, 1.0F));
    headless->context->quads()->submit(canvas, pass.get());
    return headless->picture(frame, *target, path);
}

/**
 * A projection measuring the world in pixels of the target.
 **/
glm::mat4 pixels() {
    glm::mat4 projection(1.0F);
    projection[0][0] = 2.0F / static_cast<float>(kSize);
    projection[1][1] = 2.0F / static_cast<float>(kSize);
    projection[3][0] = -1.0F;
    projection[3][1] = -1.0F;
    return projection;
}

/**
 * The square standing in the world over a stretch of the target, in pixels, far or near.
 **/
v3d::render::realtime::canvas::World::Corners square(float from, float to, float depth) {
    return { glm::vec3(from, from, depth), glm::vec3(to, from, depth), glm::vec3(to, to, depth), glm::vec3(from, to, depth) };
}

/**
 * Draw world glyphs of the square sorted by depth, through a camera measuring the world in
 * pixels, and read the target back.
 **/
boost::shared_ptr<v3d::image::Image> drawWorld(v3d::test::Headless* headless, const v3d::render::realtime::canvas::DepthOrder& order,
    const std::string& path) {
    v3d::render::realtime::canvas::World world;
    order.into(&world);
    const boost::shared_ptr<v3d::render::realtime::frame::RenderTarget> target = boost::make_shared<v3d::render::realtime::frame::RenderTarget>(*headless->context,
        v3d::render::realtime::frame::RenderTarget::Spec{ .width = kSize, .height = kSize, .colour = kColourFormat });
    v3d::render::realtime::frame::Frame frame;
    const boost::shared_ptr<v3d::render::realtime::frame::Pass> pass = frame.pass("world");
    pass->target(target);
    pass->clearColour(glm::vec4(0.0F, 0.0F, 0.0F, 1.0F));
    pass->camera(glm::mat4(1.0F), pixels());
    headless->context->worldQuads()->submit(world, pass.get());
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

/**
 * A glyph standing in the world is drawn from its curves through the pass's camera, and of two
 * glyphs sorted by depth the nearer is drawn over the further.
 **/
BOOST_AUTO_TEST_CASE(world_glyphs_are_drawn_in_depth_order) {
    v3d::test::Headless headless(kColourFormat, kSize, kSize);
    const boost::shared_ptr<v3d::render::realtime::resource::Textures> textures = headless.context->textures();
    const v3d::render::realtime::canvas::World::Glyph glyph{
        .em = { glm::vec2(0.0F, 0.0F), glm::vec2(1.0F, 0.0F), glm::vec2(1.0F, 1.0F), glm::vec2(0.0F, 1.0F) },
        .curveTexture = textures->texture(TextureFactory::Texels{ .data = kCurves.data(), .width = 8, .height = 1 }),
        .bandTexture = textures->texture(TextureFactory::Texels{ .data = kBands.data(), .width = 4, .height = 1 }) };
    v3d::render::realtime::canvas::DepthOrder order;
    // the near one added first, so only the sort puts it over the far one
    order.glyph(1.0F, square(8.0F, 40.0F, 0.25F), glyph, glm::vec4(0.0F, 1.0F, 0.0F, 1.0F));
    order.glyph(2.0F, square(0.0F, 32.0F, 0.5F), glyph, glm::vec4(1.0F, 0.0F, 0.0F, 1.0F));
    const boost::shared_ptr<v3d::image::Image> picture = drawWorld(&headless, order, "data_out/curve_square_world.png");
    BOOST_REQUIRE(picture);
    BOOST_CHECK(headless.silent());
    // the far square alone covers its own middle less a margin; the near one reaches past it
    BOOST_CHECK_EQUAL(red(*picture, 9, 9), 255);
    BOOST_CHECK_EQUAL(red(*picture, 2, 2), 0);
    // where both cover, the near one is on top
    const std::size_t overlap = ((std::size_t{22} * kSize) + 22) * 4;
    BOOST_CHECK_EQUAL(static_cast<int>(picture->data()[overlap]), 0);
    BOOST_CHECK_EQUAL(static_cast<int>(picture->data()[overlap + 1]), 255);
}

/**
 * A colour program draws its layers inside their clip in order: the gradient runs across the
 * square, and the text's colour at half its alpha is over it. Outside the square nothing is
 * drawn.
 **/
BOOST_AUTO_TEST_CASE(a_colour_program_draws_its_layers) {
    v3d::test::Headless headless(kColourFormat, kSize, kSize);
    const boost::shared_ptr<v3d::image::Image> picture = draw(&headless, 0.0F, "data_out/curve_colour_program.png", true);
    BOOST_REQUIRE(picture);
    BOOST_CHECK(headless.silent());
    const auto texel = [&picture](uint32_t x, uint32_t y) {
        const unsigned char* at = picture->data() + (((static_cast<std::size_t>(y) * kSize) + x) * 4);
        return glm::ivec3(at[0], at[1], at[2]);
    };
    // pixel 16's centre is 0.516 of an em across, so 0.526 of the way along the gradient: as
    // much red and blue as that leaves, each halved under half green
    const glm::ivec3 middle = texel(16, 16);
    BOOST_CHECK_LE(std::abs(middle.x - 60), 2);
    BOOST_CHECK_LE(std::abs(middle.y - 128), 2);
    BOOST_CHECK_LE(std::abs(middle.z - 67), 2);
    // nearer the start the gradient is redder, and nearer the end bluer
    BOOST_CHECK_GT(texel(9, 16).x, texel(9, 16).z);
    BOOST_CHECK_GT(texel(23, 16).z, texel(23, 16).x);
    BOOST_CHECK(texel(2, 16) == glm::ivec3(0));
}

BOOST_AUTO_TEST_SUITE_END()

};  // namespace
