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
 * Going through the file rather than asking the capture for its pixels is deliberate: it is
 * the same round trip moya and talyn make against their committed references, and it is what a
 * golden image comparison will do when there is one to compare against.
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

};  // namespace

BOOST_AUTO_TEST_SUITE(offscreen_frame_test)

/**
 * The whole of a frame, drawn into a target rather than a chain: a pass that clears, recorded
 * by the recorder, submitted, and read back. What it asserts is what ADR-0007 chose - that the
 * validation layer had nothing to say about any of it.
 **/
BOOST_AUTO_TEST_CASE(a_cleared_pass_is_silent_and_is_the_colour_it_cleared_to) {
    v3d::test::Headless headless(colourFormat, width, height);

    boost::shared_ptr<RenderTarget> target = boost::make_shared<RenderTarget>(headless.device, width, height, colourFormat);

    Frame frame(headless.context);
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
 * The same frame with a quad in it, which is the first case that reaches a pipeline: the
 * renderer compiles one against the target's format rather than a chain's, and the recorder
 * binds and draws it.
 **/
BOOST_AUTO_TEST_CASE(a_drawn_quad_is_silent) {
    v3d::test::Headless headless(colourFormat, width, height);

    boost::shared_ptr<RenderTarget> target = boost::make_shared<RenderTarget>(headless.device, width, height, colourFormat);

    Canvas canvas;
    canvas.resize(width, height);
    canvas.clear();
    // a quarter of the target, away from every edge, so that a wrong transform shows up as a
    // pixel that should have been background and is not
    canvas.rect(glm::vec2(16.0f, 8.0f), glm::vec2(48.0f, 24.0f), glm::vec4(1.0f, 0.0f, 0.0f, 1.0f));

    Frame frame(headless.context);
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
    headless.context->quads()->endFrame();

    BOOST_CHECK(headless.silent());

    BOOST_REQUIRE(capture.write("data_out/offscreen_quad.png"));
    boost::shared_ptr<v3d::image::Image> picture = written(headless.logger, "data_out/offscreen_quad.png");
    BOOST_REQUIRE(picture);
    // inside the quad, and outside it on all four sides. A quad drawn at the wrong scale or
    // flipped in y passes a single check in the middle and fails one of these
    BOOST_CHECK(texel(picture, 32, 16) == rgba(255, 0, 0, 255));
    BOOST_CHECK(texel(picture, 32, 2) == rgba(0, 0, 0, 255));
    BOOST_CHECK(texel(picture, 32, 29) == rgba(0, 0, 0, 255));
    BOOST_CHECK(texel(picture, 2, 16) == rgba(0, 0, 0, 255));
    BOOST_CHECK(texel(picture, 61, 16) == rgba(0, 0, 0, 255));
}

BOOST_AUTO_TEST_SUITE_END()
