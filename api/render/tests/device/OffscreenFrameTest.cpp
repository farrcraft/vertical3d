/**
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 **/

#include <api/image/Image.h>
#include <api/image/reader/Png.h>
#include <api/render/realtime/Canvas.h>
#include <api/render/realtime/Frame.h>
#include <api/render/realtime/Pass.h>
#include <api/render/realtime/vulkan/frame/Capture.h>
#include <api/render/realtime/vulkan/frame/Recorder.h>
#include <api/render/realtime/vulkan/frame/RenderTarget.h>

#include <string>
#include <vector>

#include <boost/make_shared.hpp>
#include <boost/test/unit_test.hpp>

#include "Headless.h"
#include "Reference.h"

using v3d::render::realtime::Canvas;
using v3d::render::realtime::Frame;
using v3d::render::realtime::Pass;
using v3d::render::realtime::vulkan::frame::Capture;
using v3d::render::realtime::vulkan::frame::Recorder;
using v3d::render::realtime::vulkan::frame::RenderTarget;

namespace {

const VkFormat colourFormat = VK_FORMAT_R8G8B8A8_UNORM;
const uint32_t width = 64;
const uint32_t height = 32;

/**
 * Describe a target to the recorder, the way Engine3D describes a swapchain image.
 **/
Recorder::Target describe(const boost::shared_ptr<RenderTarget>& target) {
    Recorder::Target described;
    described.image = target->image();
    described.view = target->view();
    described.extent = target->extent();
    // not presented, and PRESENT_SRC is not a layout a device with no swapchain extension has
    described.finalLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
    return described;
}

/**
 * Read back what a capture wrote.
 *
 * This reads the file rather than asking the capture for its pixels. The reference comparison
 * makes the same round trip, so a case that checks a colour by hand and one that checks a
 * picture read the same bytes.
 **/
boost::shared_ptr<v3d::image::Image> written(const boost::shared_ptr<v3d::log::Logger>& logger, const std::string& path) {
    v3d::image::reader::Png png(logger);
    return png.read(path);
}

/**
 * @return the texel at (x, y), as the four bytes RGBA
 **/
std::vector<unsigned char> texel(const boost::shared_ptr<v3d::image::Image>& image, uint32_t x, uint32_t y) {
    const unsigned char* pixels = image->data();
    const std::size_t at = (static_cast<std::size_t>(y) * image->width() + x) * 4;
    return std::vector<unsigned char>(pixels + at, pixels + at + 4);
}

/**
 * @return the four bytes a colour channel quartet should read as
 **/
std::vector<unsigned char> rgba(unsigned char r, unsigned char g, unsigned char b, unsigned char a) {
    return std::vector<unsigned char>{r, g, b, a};
}

/**
 * Fill a target of the given format with a #808080 quad, and read back the texel in its middle.
 *
 * The quad renderer is built against the context's format, so the context is made for the
 * target's. The capture converts the stored bytes as they are, without decoding sRGB, so it reads
 * what the target holds.
 **/
std::vector<unsigned char> greyQuad(VkFormat format, const std::string& path) {
    v3d::test::Headless headless(format, width, height);
    boost::shared_ptr<RenderTarget> target = boost::make_shared<RenderTarget>(headless.device, headless.context->ring(), width, height, format);

    Canvas canvas;
    canvas.resize(width, height);
    canvas.clear();
    const float grey = 128.0f / 255.0f;
    canvas.rect(glm::vec2(0.0f, 0.0f), glm::vec2(static_cast<float>(width), static_cast<float>(height)),
        glm::vec4(grey, grey, grey, 1.0f));

    Frame frame;
    boost::shared_ptr<Pass> pass = frame.pass("colour");
    pass->clearColour(glm::vec4(0.0f, 0.0f, 0.0f, 1.0f));
    headless.context->quads()->submit(canvas, pass.get());

    Capture capture(headless.device, headless.logger);
    VkCommandBuffer commands = headless.context->ring()->begin();
    Recorder::record(commands, frame, describe(target), *headless.context->resources(), headless.context->frameUniforms().get());
    Capture::Source source;
    source.image = target->image();
    source.extent = target->extent();
    source.format = target->format();
    source.layout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
    capture.record(commands, source);
    headless.submitAndWait(commands);

    BOOST_CHECK(headless.silent());
    BOOST_REQUIRE(capture.write(path));
    boost::shared_ptr<v3d::image::Image> picture = written(headless.logger, path);
    BOOST_REQUIRE(picture);
    return texel(picture, width / 2, height / 2);
}

/**
 * @return whether a byte read back is within one of 0x80. The sRGB encode on store may round
 *         either way, and an unconverted grey stores about 0xBC
 **/
bool nearGrey(unsigned char value) {
    return value >= 0x7F && value <= 0x81;
}

};  // namespace

BOOST_AUTO_TEST_SUITE(offscreen_frame_test)

/**
 * A complete frame, drawn into a target rather than a chain: a pass that clears, recorded by
 * the recorder, submitted, and read back. The case passes if the validation layer reports no
 * errors and every texel is the clear colour.
 **/
BOOST_AUTO_TEST_CASE(a_cleared_pass_is_silent_and_is_the_colour_it_cleared_to) {
    v3d::test::Headless headless(colourFormat, width, height);

    boost::shared_ptr<RenderTarget> target = boost::make_shared<RenderTarget>(headless.device, headless.context->ring(), width, height, colourFormat);

    Frame frame;
    boost::shared_ptr<Pass> pass = frame.pass("colour");
    pass->clearColour(glm::vec4(0.0f, 1.0f, 0.0f, 1.0f));

    Capture capture(headless.device, headless.logger);

    VkCommandBuffer commands = headless.context->ring()->begin();
    Recorder::Target described = describe(target);
    Recorder::record(commands, frame, described, *headless.context->resources(), headless.context->frameUniforms().get());

    Capture::Source source;
    source.image = target->image();
    source.extent = target->extent();
    source.format = target->format();
    // what the frame was told to leave it in, since it is not presented
    source.layout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
    capture.record(commands, source);

    headless.submitAndWait(commands);

    BOOST_CHECK(headless.silent());

    BOOST_REQUIRE(capture.write("data_out/offscreen_cleared.png"));
    boost::shared_ptr<v3d::image::Image> picture = written(headless.logger, "data_out/offscreen_cleared.png");
    BOOST_REQUIRE(picture);
    BOOST_CHECK_EQUAL(picture->width(), width);
    BOOST_CHECK_EQUAL(picture->height(), height);
    // every texel, not a sample of them: a clear that reached only part of the target is the
    // kind of wrong a spot check in the middle would pass
    for (uint32_t y = 0; y < height; y++) {
        for (uint32_t x = 0; x < width; x++) {
            if (texel(picture, x, y) != rgba(0, 255, 0, 255)) {
                BOOST_ERROR("texel " << x << "," << y << " is not the colour the pass cleared to");
                return;
            }
        }
    }
}

/**
 * The same frame with a quad in it, so the case reaches a pipeline: the renderer compiles one
 * against the target's format rather than a chain's, and the recorder binds and draws it.
 *
 * The picture is compared against a committed reference, so it is deliberately simple: one
 * flat rect on a cleared target, at integer boundaries, in channels at the ends of their
 * range. The Vulkan specification fixes that output exactly, so any conformant driver
 * produces it bit for bit.
 **/
BOOST_AUTO_TEST_CASE(a_drawn_quad_is_silent_and_is_the_committed_picture) {
    v3d::test::Headless headless(colourFormat, width, height);

    boost::shared_ptr<RenderTarget> target = boost::make_shared<RenderTarget>(headless.device, headless.context->ring(), width, height, colourFormat);

    Canvas canvas;
    canvas.resize(width, height);
    canvas.clear();
    // a quarter of the target, away from every edge, so that a wrong transform shows up as a
    // pixel that should have been background and is not
    canvas.rect(glm::vec2(16.0f, 8.0f), glm::vec2(48.0f, 24.0f), glm::vec4(1.0f, 0.0f, 0.0f, 1.0f));

    Frame frame;
    boost::shared_ptr<Pass> pass = frame.pass("colour");
    pass->clearColour(glm::vec4(0.0f, 0.0f, 0.0f, 1.0f));

    headless.context->quads()->submit(canvas, pass.get());

    Capture capture(headless.device, headless.logger);

    VkCommandBuffer commands = headless.context->ring()->begin();
    Recorder::Target described = describe(target);
    Recorder::record(commands, frame, described, *headless.context->resources(), headless.context->frameUniforms().get());

    Capture::Source source;
    source.image = target->image();
    source.extent = target->extent();
    source.format = target->format();
    source.layout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
    capture.record(commands, source);

    headless.submitAndWait(commands);

    BOOST_CHECK(headless.silent());

    // every texel rather than a few spot checks, so a quad drawn at the wrong scale, flipped in
    // y or off by a pixel fails wherever a texel differs from the reference
    v3d::test::checkReference(headless.logger, &capture, "quad");
}


/**
 * A quad drawn with a texture the case uploads. This is the only check of the upload path: a
 * texture that arrived transposed, mirrored, in the wrong channel order or in the wrong mip
 * gives a wrong picture rather than a validation error.
 *
 * The texture is built here rather than committed beside the reference, so there is no second
 * file to keep in step with the picture. Its four quadrants are four different full range
 * colours over a rectangle that is wider than it is tall, so a transpose and a flip in either
 * axis all give different pictures.
 *
 * It is drawn at one texel per pixel on integer boundaries, so the output is exact. Every
 * sampler in the tree is linear, and at that scale the filter lands on texel centres, so
 * each texel reaches the target unchanged.
 **/
BOOST_AUTO_TEST_CASE(a_textured_quad_is_the_texture_that_was_uploaded) {
    v3d::test::Headless headless(colourFormat, width, height);

    boost::shared_ptr<RenderTarget> target = boost::make_shared<RenderTarget>(headless.device, headless.context->ring(), width, height, colourFormat);

    // 32 by 16, which is the size the quad below covers in pixels
    const uint32_t textureWidth = 32;
    const uint32_t textureHeight = 16;
    std::vector<unsigned char> texels(static_cast<std::size_t>(textureWidth) * textureHeight * 4);
    for (uint32_t y = 0; y < textureHeight; y++) {
        for (uint32_t x = 0; x < textureWidth; x++) {
            const bool right = x >= textureWidth / 2;
            const bool lower = y >= textureHeight / 2;
            const std::size_t at = (static_cast<std::size_t>(y) * textureWidth + x) * 4;
            texels[at + 0] = static_cast<unsigned char>(right && !lower ? 0 : 255);
            texels[at + 1] = static_cast<unsigned char>(lower ? 255 : 0);
            texels[at + 2] = static_cast<unsigned char>(right ? 255 : 0);
            texels[at + 3] = 255;
        }
    }
    const v3d::render::realtime::TextureHandle uploaded =
        headless.context->textures()->texture(texels.data(), textureWidth, textureHeight, 4);
    BOOST_REQUIRE(uploaded.valid());

    Canvas canvas;
    canvas.resize(width, height);
    canvas.clear();
    // white, so the vertex colour multiplies the texel by one and the picture is the texture
    canvas.rect(glm::vec2(16.0f, 8.0f), glm::vec2(48.0f, 24.0f),
        glm::vec2(0.0f, 0.0f), glm::vec2(1.0f, 1.0f), glm::vec4(1.0f, 1.0f, 1.0f, 1.0f), uploaded);

    Frame frame;
    boost::shared_ptr<Pass> pass = frame.pass("colour");
    pass->clearColour(glm::vec4(0.0f, 0.0f, 0.0f, 1.0f));

    headless.context->quads()->submit(canvas, pass.get());

    Capture capture(headless.device, headless.logger);

    VkCommandBuffer commands = headless.context->ring()->begin();
    Recorder::Target described = describe(target);
    Recorder::record(commands, frame, described, *headless.context->resources(), headless.context->frameUniforms().get());

    Capture::Source source;
    source.image = target->image();
    source.extent = target->extent();
    source.format = target->format();
    source.layout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
    capture.record(commands, source);

    headless.submitAndWait(commands);

    BOOST_CHECK(headless.silent());

    v3d::test::checkReference(headless.logger, &capture, "textured_quad");
}

/**
 * The same clear, on a device whose memory comes from a suballocator rather than from one
 * device allocation per resource. Nothing in this tree selects that allocator, because direct
 * allocation is the default. This case is the only code that runs the suballocated path.
 *
 * It asserts the picture as well as the silence, because an allocation bound at the wrong
 * offset is a wrong picture rather than a reported error - a suballocated region starts part
 * way into its block, and the direct path's offset is always zero.
 **/
BOOST_AUTO_TEST_CASE(a_suballocated_device_clears_the_same_way) {
    v3d::test::Headless headless(colourFormat, width, height,
        v3d::render::realtime::vulkan::memory::Allocator::Kind::Suballocated);
    BOOST_REQUIRE(headless.device->allocator().kind() ==
        v3d::render::realtime::vulkan::memory::Allocator::Kind::Suballocated);

    boost::shared_ptr<RenderTarget> target = boost::make_shared<RenderTarget>(headless.device, headless.context->ring(), width, height, colourFormat);

    Frame frame;
    boost::shared_ptr<Pass> pass = frame.pass("colour");
    pass->clearColour(glm::vec4(0.0f, 1.0f, 0.0f, 1.0f));

    Capture capture(headless.device, headless.logger);

    VkCommandBuffer commands = headless.context->ring()->begin();
    Recorder::Target described = describe(target);
    Recorder::record(commands, frame, described, *headless.context->resources(), headless.context->frameUniforms().get());

    Capture::Source source;
    source.image = target->image();
    source.extent = target->extent();
    source.format = target->format();
    source.layout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
    capture.record(commands, source);

    headless.submitAndWait(commands);

    BOOST_CHECK(headless.silent());

    BOOST_REQUIRE(capture.write("data_out/offscreen_suballocated.png"));
    boost::shared_ptr<v3d::image::Image> picture = written(headless.logger, "data_out/offscreen_suballocated.png");
    BOOST_REQUIRE(picture);
    for (uint32_t y = 0; y < height; y++) {
        for (uint32_t x = 0; x < width; x++) {
            if (texel(picture, x, y) != rgba(0, 255, 0, 255)) {
                BOOST_ERROR("texel " << x << "," << y << " is not the colour the pass cleared to");
                return;
            }
        }
    }
}

/**
 * A pass drawing into part of its target, with a clip rectangle that reaches outside that part.
 * The scissor is clamped to the pass's region, so the layer reports nothing, and only the
 * pixels inside both the clip and the region are drawn. The rest of the region is the clear
 * colour. Pixels outside the region are undefined and are not checked.
 **/
BOOST_AUTO_TEST_CASE(a_clip_is_cut_to_the_pass_viewport) {
    v3d::test::Headless headless(colourFormat, width, height);
    boost::shared_ptr<RenderTarget> target = boost::make_shared<RenderTarget>(headless.device, headless.context->ring(), width, height, colourFormat);

    // the region is the middle of the target: 32 by 16 at 16, 8
    const uint32_t regionX = 16;
    const uint32_t regionY = 8;
    const uint32_t regionWidth = 32;
    const uint32_t regionHeight = 16;

    // the canvas covers the region, and the clip is in the image's pixels: 0,0 to 24,12 overlaps
    // the region only from 16,8 to 24,12
    Canvas canvas;
    canvas.resize(regionWidth, regionHeight);
    canvas.clear();
    canvas.clip(glm::vec2(0.0f, 0.0f), glm::vec2(24.0f, 12.0f));
    canvas.rect(glm::vec2(0.0f, 0.0f), glm::vec2(static_cast<float>(regionWidth), static_cast<float>(regionHeight)),
        glm::vec4(1.0f, 0.0f, 0.0f, 1.0f));
    canvas.unclip();

    Frame frame;
    boost::shared_ptr<Pass> pass = frame.pass("colour");
    pass->viewport(glm::vec4(static_cast<float>(regionX), static_cast<float>(regionY), static_cast<float>(regionWidth),
        static_cast<float>(regionHeight)));
    pass->clearColour(glm::vec4(0.0f, 1.0f, 0.0f, 1.0f));
    headless.context->quads()->submit(canvas, pass.get());

    Capture capture(headless.device, headless.logger);
    VkCommandBuffer commands = headless.context->ring()->begin();
    Recorder::record(commands, frame, describe(target), *headless.context->resources(), headless.context->frameUniforms().get());
    Capture::Source source;
    source.image = target->image();
    source.extent = target->extent();
    source.format = target->format();
    source.layout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
    capture.record(commands, source);
    headless.submitAndWait(commands);

    BOOST_CHECK(headless.silent());

    BOOST_REQUIRE(capture.write("data_out/offscreen_clipped_viewport.png"));
    boost::shared_ptr<v3d::image::Image> picture = written(headless.logger, "data_out/offscreen_clipped_viewport.png");
    BOOST_REQUIRE(picture);
    for (uint32_t y = regionY; y < regionY + regionHeight; y++) {
        for (uint32_t x = regionX; x < regionX + regionWidth; x++) {
            const bool clipped = x < 24 && y < 12;
            const std::vector<unsigned char> expected = clipped ? rgba(255, 0, 0, 255) : rgba(0, 255, 0, 255);
            if (texel(picture, x, y) != expected) {
                BOOST_ERROR("texel " << x << "," << y << " should be " << (clipped ? "the quad" : "the clear colour"));
                return;
            }
        }
    }
}

/**
 * A frame that is begun and never submitted, as when recording throws, leaves its fence
 * signalled. Beginning the same slot again then returns rather than waiting forever, and the
 * frame after it draws and is silent.
 **/
BOOST_AUTO_TEST_CASE(an_abandoned_frame_can_be_begun_again) {
    v3d::test::Headless headless(colourFormat, width, height);
    VkCommandBuffer abandoned = headless.context->ring()->begin();
    BOOST_REQUIRE(abandoned != VK_NULL_HANDLE);

    const uint64_t begun = headless.context->ring()->begun();
    VkCommandBuffer commands = headless.context->ring()->begin();
    BOOST_REQUIRE(commands == abandoned);
    // the frame begun again is the same frame, so it is not counted twice, and nothing retired
    // is collected a frame early
    BOOST_CHECK_EQUAL(headless.context->ring()->begun(), begun);
    headless.submitAndWait(commands);
    BOOST_CHECK(headless.silent());

    // once that frame is submitted, the next begin is a new frame
    headless.submitAndWait(headless.context->ring()->begin());
    BOOST_CHECK_EQUAL(headless.context->ring()->begun(), begun + 1);
}

/**
 * Something retired between a submit and the next begin may be named by items queued for the
 * frame about to begin. It therefore outlives that frame, and is destroyed only once
 * framesInFlight frames after it have begun. Something retired while a frame is being recorded
 * is destroyed once framesInFlight frames after that one have begun.
 **/
BOOST_AUTO_TEST_CASE(a_retirement_outlives_the_frame_about_to_begin) {
    // declared before the device, because the ring runs whatever is still held when it goes
    bool queued = false;
    bool recording = false;
    v3d::test::Headless headless(colourFormat, width, height);
    const boost::shared_ptr<v3d::render::realtime::vulkan::frame::Ring> ring = headless.context->ring();
    headless.submitAndWait(ring->begin());

    ring->retire([&queued]() { queued = true; });
    for (uint32_t frame = 0; frame < ring->framesInFlight(); frame++) {
        headless.submit(ring->begin());
        BOOST_CHECK(!queued);
    }
    headless.submit(ring->begin());
    BOOST_CHECK(queued);

    VkCommandBuffer commands = ring->begin();
    ring->retire([&recording]() { recording = true; });
    headless.submit(commands);
    for (uint32_t frame = 1; frame < ring->framesInFlight(); frame++) {
        headless.submit(ring->begin());
        BOOST_CHECK(!recording);
    }
    headless.submit(ring->begin());
    BOOST_CHECK(recording);

    ring->waitIdle();
    BOOST_CHECK(headless.silent());
}

/**
 * A quad's colour is the colour that appears, whether the target stores it as written or encodes
 * it as sRGB. A #808080 quad reads back as 0x80 from an _SRGB target, which it does only because
 * the quad was decoded to linear before the target encoded it.
 **/
BOOST_AUTO_TEST_CASE(a_grey_quad_reads_back_grey_from_an_srgb_target) {
    const std::vector<unsigned char> centre = greyQuad(VK_FORMAT_R8G8B8A8_SRGB, "data_out/quad_grey_srgb.png");
    BOOST_TEST_MESSAGE("centre " << int(centre[0]) << "," << int(centre[1]) << "," << int(centre[2]));
    BOOST_TEST(nearGrey(centre[0]));
    BOOST_TEST(nearGrey(centre[1]));
    BOOST_TEST(nearGrey(centre[2]));
}

/**
 * The same quad reads back as 0x80 from a UNORM target, which stores what it is given. A quad
 * decoded whatever its target would read about 0x37 here, darkening every UNORM app's ui.
 **/
BOOST_AUTO_TEST_CASE(a_grey_quad_reads_back_grey_from_a_unorm_target) {
    const std::vector<unsigned char> centre = greyQuad(VK_FORMAT_R8G8B8A8_UNORM, "data_out/quad_grey_unorm.png");
    BOOST_TEST_MESSAGE("centre " << int(centre[0]) << "," << int(centre[1]) << "," << int(centre[2]));
    BOOST_TEST(centre[0] == 0x80);
    BOOST_TEST(centre[1] == 0x80);
    BOOST_TEST(centre[2] == 0x80);
}

BOOST_AUTO_TEST_SUITE_END()
