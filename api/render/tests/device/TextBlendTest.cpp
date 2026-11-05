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
#include <api/render/realtime/canvas/FieldRamp.h>
#include <api/render/realtime/canvas/World.h>

#include <vulkan/vulkan.h>

#include <cstddef>
#include <cstdint>
#include <cstdlib>
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

const uint32_t kSize = 8;

/**
 * Half coverage drawn in white over black: the stored value of a display space blend.
 **/
constexpr int kDisplayHalf = 128;

/**
 * The same blended in linear light and encoded: what an _SRGB target stores uncorrected.
 **/
constexpr int kLinearHalf = 188;

/**
 * A target of a format, cleared to black, that a frame draws one pass into.
 **/
struct Drawn {
    boost::shared_ptr<v3d::render::realtime::frame::RenderTarget> target;
    v3d::render::realtime::frame::Frame frame;
    boost::shared_ptr<v3d::render::realtime::frame::Pass> pass;

    Drawn(v3d::test::Headless* headless, VkFormat format) :
        target(boost::make_shared<v3d::render::realtime::frame::RenderTarget>(*headless->context,
            v3d::render::realtime::frame::RenderTarget::Spec{ .width = kSize, .height = kSize, .colour = format })),
        pass(frame.pass("text")) {
        pass->target(target);
        pass->clearColour(glm::vec4(0.0F, 0.0F, 0.0F, 1.0F));
    }
};

/**
 * A page whose every texel is half covered, or a field whose every value is its middle.
 **/
TextureHandle half(v3d::test::Headless* headless) {
    const std::vector<unsigned char> pixels(static_cast<std::size_t>(kSize) * kSize, 128);
    return headless->context->textures()->texture(pixels.data(), kSize, kSize, 1);
}

/**
 * Draw a half covered page over the whole target as a quad with a contrast, white, and read the
 * red of its middle back.
 **/
int coverage(VkFormat format, float contrast, const std::string& path) {
    v3d::test::Headless headless(format, kSize, kSize);
    const TextureHandle page = half(&headless);
    Canvas canvas;
    canvas.resize(glm::uvec2(kSize, kSize));
    canvas.clear();
    const auto side = static_cast<float>(kSize);
    const glm::vec4 white(1.0F);
    const std::vector<Canvas::Vertex> corners{
        { .position = glm::vec2(0.0F, 0.0F), .uv = glm::vec2(0.0F, 0.0F), .colour = white, .contrast = contrast },
        { .position = glm::vec2(0.0F, side), .uv = glm::vec2(0.0F, 1.0F), .colour = white, .contrast = contrast },
        { .position = glm::vec2(side, side), .uv = glm::vec2(1.0F, 1.0F), .colour = white, .contrast = contrast },
        { .position = glm::vec2(side, 0.0F), .uv = glm::vec2(1.0F, 0.0F), .colour = white, .contrast = contrast } };
    canvas.glyphs(corners, std::vector<uint32_t>{ 0, 1, 2, 2, 3, 0 }, page);

    const Drawn drawn(&headless, format);
    headless.context->quads()->submit(canvas, drawn.pass.get());
    const boost::shared_ptr<v3d::image::Image> picture = headless.picture(drawn.frame, *drawn.target, path);
    BOOST_REQUIRE(picture);
    BOOST_CHECK(headless.silent());
    return picture->data()[((static_cast<std::size_t>(kSize / 2) * kSize) + (kSize / 2)) * 4];
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
 * Stand a field of middle values in the world over the whole target, faded across a ramp so its
 * coverage is a half, white with a contrast, and read the red of its middle back.
 **/
int worldCoverage(VkFormat format, float contrast, const std::string& path) {
    v3d::test::Headless headless(format, kSize, kSize);
    const TextureHandle field = half(&headless);
    const auto side = static_cast<float>(kSize);
    v3d::render::realtime::canvas::World world;
    world.field({ glm::vec3(0.0F, 0.0F, 0.5F), glm::vec3(side, 0.0F, 0.5F), glm::vec3(side, side, 0.5F), glm::vec3(0.0F, side, 0.5F) },
        glm::vec2(0.0F), glm::vec2(1.0F), glm::vec4(1.0F), field, v3d::render::realtime::canvas::fieldRamp(0.75F, 0.25F), contrast);

    const Drawn drawn(&headless, format);
    drawn.pass->camera(glm::mat4(1.0F), pixels());
    headless.context->worldQuads()->submit(world, drawn.pass.get());
    const boost::shared_ptr<v3d::image::Image> picture = headless.picture(drawn.frame, *drawn.target, path);
    BOOST_REQUIRE(picture);
    BOOST_CHECK(headless.silent());
    return picture->data()[((static_cast<std::size_t>(kSize / 2) * kSize) + (kSize / 2)) * 4];
}

/**
 * @return whether a stored value is within a few steps of another
 **/
bool near(int stored, int expected) {
    return std::abs(stored - expected) <= 3;
}

BOOST_AUTO_TEST_SUITE(text_blend_test)

/**
 * Half covered text stores the same value in a UNORM target and in an _SRGB one, which blends in
 * linear light and would otherwise store it much lighter.
 **/
BOOST_AUTO_TEST_CASE(text_has_one_weight_in_either_target) {
    const int unorm = coverage(VK_FORMAT_R8G8B8A8_UNORM, 0.0F, "data_out/text_blend_unorm.png");
    const int srgb = coverage(VK_FORMAT_R8G8B8A8_SRGB, 0.0F, "data_out/text_blend_srgb.png");
    BOOST_TEST_MESSAGE("unorm " << unorm << ", srgb " << srgb);
    BOOST_CHECK(near(unorm, kDisplayHalf));
    BOOST_CHECK(near(srgb, kDisplayHalf));
}

/**
 * A quad that is not text is blended as the target blends it, so a picture in an _SRGB target is
 * as it was.
 **/
BOOST_AUTO_TEST_CASE(a_picture_is_not_corrected) {
    BOOST_CHECK(near(coverage(VK_FORMAT_R8G8B8A8_SRGB, -1.0F, "data_out/text_blend_picture.png"), kLinearHalf));
}

/**
 * Contrast strengthens coverage in either target: at full contrast, half coverage is three
 * quarters.
 **/
BOOST_AUTO_TEST_CASE(contrast_strengthens_coverage) {
    BOOST_CHECK(near(coverage(VK_FORMAT_R8G8B8A8_UNORM, 1.0F, "data_out/text_blend_contrast_unorm.png"), 191));
    BOOST_CHECK(near(coverage(VK_FORMAT_R8G8B8A8_SRGB, 1.0F, "data_out/text_blend_contrast_srgb.png"), 191));
}

/**
 * Text standing in the world is corrected as text on the screen is.
 **/
BOOST_AUTO_TEST_CASE(world_text_has_one_weight_in_either_target) {
    const int unorm = worldCoverage(VK_FORMAT_R8G8B8A8_UNORM, 0.0F, "data_out/text_blend_world_unorm.png");
    const int srgb = worldCoverage(VK_FORMAT_R8G8B8A8_SRGB, 0.0F, "data_out/text_blend_world_srgb.png");
    BOOST_TEST_MESSAGE("unorm " << unorm << ", srgb " << srgb);
    BOOST_CHECK(near(unorm, kDisplayHalf));
    BOOST_CHECK(near(srgb, kDisplayHalf));
}

BOOST_AUTO_TEST_SUITE_END()

};  // namespace
