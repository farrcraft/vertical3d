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
#include <api/render/realtime/canvas/FieldRamp.h>
#include <api/render/realtime/canvas/World.h>

#include <vulkan/vulkan.h>

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

using v3d::render::realtime::TextureHandle;
using v3d::render::realtime::canvas::Canvas;
using v3d::render::realtime::canvas::fieldRamp;

const VkFormat kColourFormat = VK_FORMAT_R8G8B8A8_UNORM;
const uint32_t kSize = 32;

/**
 * A field whose value rises from 0 at the left to 1 at the right, a texel for each pixel of the
 * target, so the value at pixel x is x / 31.
 **/
TextureHandle ramp(v3d::test::Headless* headless) {
    std::vector<unsigned char> pixels(static_cast<std::size_t>(kSize) * kSize);
    for (uint32_t y = 0; y < kSize; y++) {
        for (uint32_t x = 0; x < kSize; x++) {
            pixels[(static_cast<std::size_t>(y) * kSize) + x] = static_cast<unsigned char>((x * 255) / (kSize - 1));
        }
    }
    return headless->context->textures()->texture(pixels.data(), kSize, kSize, 1);
}

/**
 * Draw the field over the whole target as text with a ramp, in white, and read the target back.
 **/
boost::shared_ptr<v3d::image::Image> draw(v3d::test::Headless* headless, uint32_t ramped, const std::string& path) {
    const TextureHandle field = ramp(headless);
    Canvas canvas;
    canvas.resize(glm::uvec2(kSize, kSize));
    canvas.clear();
    const auto side = static_cast<float>(kSize);
    const std::vector<Canvas::Vertex> corners{
        { .position = glm::vec2(0.0F, 0.0F), .uv = glm::vec2(0.0F, 0.0F), .colour = glm::vec4(1.0F), .glyph = ramped },
        { .position = glm::vec2(side, 0.0F), .uv = glm::vec2(1.0F, 0.0F), .colour = glm::vec4(1.0F), .glyph = ramped },
        { .position = glm::vec2(side, side), .uv = glm::vec2(1.0F, 1.0F), .colour = glm::vec4(1.0F), .glyph = ramped },
        { .position = glm::vec2(0.0F, side), .uv = glm::vec2(0.0F, 1.0F), .colour = glm::vec4(1.0F), .glyph = ramped } };
    canvas.text(corners, std::vector<uint32_t>{ 0, 1, 2, 2, 3, 0 }, field);

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
 * Stand the field in the world over the whole target with a ramp, through a camera measuring the
 * world in pixels, and read the target back.
 **/
boost::shared_ptr<v3d::image::Image> drawWorld(v3d::test::Headless* headless, uint32_t ramped, const std::string& path) {
    const TextureHandle field = ramp(headless);
    const auto side = static_cast<float>(kSize);
    v3d::render::realtime::canvas::World world;
    world.field({ glm::vec3(0.0F, 0.0F, 0.5F), glm::vec3(side, 0.0F, 0.5F), glm::vec3(side, side, 0.5F), glm::vec3(0.0F, side, 0.5F) },
        glm::vec2(0.0F, 0.0F), glm::vec2(1.0F, 1.0F), glm::vec4(1.0F), field, ramped);

    const boost::shared_ptr<v3d::render::realtime::RenderTarget> target = boost::make_shared<v3d::render::realtime::RenderTarget>(*headless->context,
        v3d::render::realtime::RenderTarget::Spec{ .width = kSize, .height = kSize, .colour = kColourFormat });
    v3d::render::realtime::Frame frame;
    const boost::shared_ptr<v3d::render::realtime::Pass> pass = frame.pass("world");
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

BOOST_AUTO_TEST_SUITE(field_effect_test)

/**
 * With no ramp, a field is a glyph: whole above its middle value and empty below it.
 **/
BOOST_AUTO_TEST_CASE(a_field_with_no_ramp_is_its_glyph) {
    v3d::test::Headless headless(kColourFormat, kSize, kSize);
    const boost::shared_ptr<v3d::image::Image> picture = draw(&headless, 0, "data_out/field_glyph.png");
    BOOST_REQUIRE(picture);
    BOOST_CHECK(headless.silent());
    BOOST_CHECK_EQUAL(red(*picture, 12, 16), 0);
    BOOST_CHECK_EQUAL(red(*picture, 19, 16), 255);
}

/**
 * A soft ramp is whole above its first value, empty below its second, and partly covered
 * between them, rising as the field does.
 **/
BOOST_AUTO_TEST_CASE(a_soft_ramp_fades_between_its_values) {
    v3d::test::Headless headless(kColourFormat, kSize, kSize);
    const boost::shared_ptr<v3d::image::Image> picture = draw(&headless, fieldRamp(0.7F, 0.3F), "data_out/field_soft.png");
    BOOST_REQUIRE(picture);
    BOOST_CHECK(headless.silent());
    BOOST_CHECK_EQUAL(red(*picture, 6, 16), 0);
    BOOST_CHECK_EQUAL(red(*picture, 25, 16), 255);
    const int low = red(*picture, 12, 16);
    const int high = red(*picture, 19, 16);
    BOOST_CHECK(low > 0 && low < high && high < 255);
}

/**
 * A hard ramp moves the edge to its value: an outline reaching past the glyph's edge covers
 * pixels the glyph does not.
 **/
BOOST_AUTO_TEST_CASE(a_hard_ramp_moves_the_edge) {
    v3d::test::Headless headless(kColourFormat, kSize, kSize);
    const boost::shared_ptr<v3d::image::Image> picture = draw(&headless, fieldRamp(0.3F, 0.3F), "data_out/field_hard.png");
    BOOST_REQUIRE(picture);
    BOOST_CHECK(headless.silent());
    BOOST_CHECK_EQUAL(red(*picture, 7, 16), 0);
    BOOST_CHECK_EQUAL(red(*picture, 12, 16), 255);
}

/**
 * A field standing in the world is drawn with its ramp as on the screen.
 **/
BOOST_AUTO_TEST_CASE(a_field_in_the_world_is_drawn_with_its_ramp) {
    v3d::test::Headless headless(kColourFormat, kSize, kSize);
    const boost::shared_ptr<v3d::image::Image> picture = drawWorld(&headless, fieldRamp(0.7F, 0.3F), "data_out/field_world.png");
    BOOST_REQUIRE(picture);
    BOOST_CHECK(headless.silent());
    BOOST_CHECK_EQUAL(red(*picture, 6, 16), 0);
    BOOST_CHECK_EQUAL(red(*picture, 25, 16), 255);
    const int middle = red(*picture, 16, 16);
    BOOST_CHECK(middle > 0 && middle < 255);
}

BOOST_AUTO_TEST_SUITE_END()

};  // namespace
