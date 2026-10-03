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
using v3d::render::realtime::TextureHandle;
using v3d::render::realtime::vulkan::frame::Capture;
using v3d::render::realtime::vulkan::frame::Recorder;
using v3d::render::realtime::vulkan::frame::RenderTarget;

namespace {

const VkFormat colourFormat = VK_FORMAT_R8G8B8A8_UNORM;
const uint32_t width = 16;
const uint32_t height = 16;

/**
 * A texture that is one flat colour, so that wherever and however it is sampled what reaches
 * the target is that colour exactly - ADR-0054.
 **/
TextureHandle flat(v3d::test::Headless* headless, unsigned char r, unsigned char g, unsigned char b) {
    std::vector<unsigned char> pixels;
    for (int texel = 0; texel < 4 * 4; texel++) {
        pixels.insert(pixels.end(), {r, g, b, 0xFF});
    }
    return headless->context->quads()->texture(pixels.data(), 4, 4, 4);
}

/**
 * Record a frame that covers the target with a texture, and submit it without waiting.
 * @param capture where to read the target back into, if anywhere
 **/
void draw(v3d::test::Headless* headless, const boost::shared_ptr<RenderTarget>& target,
    const TextureHandle& texture, Capture* capture) {
    Canvas canvas;
    canvas.resize(width, height);
    canvas.clear();
    canvas.rect(glm::vec2(0.0f, 0.0f), glm::vec2(static_cast<float>(width), static_cast<float>(height)), glm::vec2(0.0f, 0.0f), glm::vec2(1.0f, 1.0f),
        glm::vec4(1.0f), texture);

    Frame frame(headless->context);
    boost::shared_ptr<Pass> pass = frame.pass("colour");
    pass->clearColour(glm::vec4(0.0f, 0.0f, 0.0f, 1.0f));
    headless->context->quads()->submit(canvas, pass.get());

    VkCommandBuffer commands = headless->context->ring()->begin();
    Recorder::Target described;
    described.image = target->image();
    described.view = target->view();
    described.extent = target->extent();
    described.finalLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
    Recorder::record(commands, frame, described, *headless->context->resources(),
        headless->context->frameUniforms().get());

    if (capture != nullptr) {
        Capture::Source source;
        source.image = target->image();
        source.extent = target->extent();
        source.format = target->format();
        source.layout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
        capture->record(commands, source);
    }

    headless->submit(commands);
    headless->context->quads()->endFrame();
}

/**
 * Begin and submit a frame that draws nothing, which is all it takes to move the ring on.
 **/
void idle(v3d::test::Headless* headless) {
    headless->submit(headless->context->ring()->begin());
}

};  // namespace

BOOST_AUTO_TEST_SUITE(release_test)

/**
 * A texture released while the frame that samples it is still in flight is not destroyed
 * under that frame - ADR-0061. Destroying it at once is a use after free on the device,
 * which nothing reports but the validation layer, so a silent log is the assertion.
 *
 * The layer only learns that a frame has finished when the app waits on its fence, so the
 * frame is in flight as far as it is concerned however quickly the device drew it.
 **/
BOOST_AUTO_TEST_CASE(a_texture_released_in_flight_outlives_its_frame) {
    v3d::test::Headless headless(colourFormat, width, height);
    boost::shared_ptr<RenderTarget> target = boost::make_shared<RenderTarget>(headless.device, width, height, colourFormat);

    TextureHandle texture = flat(&headless, 0xFF, 0x00, 0x00);
    draw(&headless, target, texture, nullptr);

    BOOST_CHECK(headless.context->quads()->release(texture));
    BOOST_CHECK(headless.context->resources()->texture(texture) == nullptr);

    for (int frame = 0; frame < 3; frame++) {
        idle(&headless);
    }
    headless.context->ring()->waitIdle();

    BOOST_CHECK(headless.silent());
}

/**
 * A texture registered after a release reuses the released slot, and draws as itself rather
 * than as the texture that was in the slot before. A material looked up by slot alone would
 * hand back the old descriptor set, naming an image that has been destroyed.
 **/
BOOST_AUTO_TEST_CASE(a_reused_slot_draws_its_new_texture) {
    v3d::test::Headless headless(colourFormat, width, height);
    boost::shared_ptr<RenderTarget> target = boost::make_shared<RenderTarget>(headless.device, width, height, colourFormat);

    TextureHandle red = flat(&headless, 0xFF, 0x00, 0x00);
    draw(&headless, target, red, nullptr);
    BOOST_REQUIRE(headless.context->quads()->release(red));

    TextureHandle green = flat(&headless, 0x00, 0xFF, 0x00);
    BOOST_REQUIRE_EQUAL(green.id(), red.id());

    Capture capture(headless.device, headless.logger);
    draw(&headless, target, green, &capture);
    headless.context->ring()->waitIdle();

    BOOST_CHECK(headless.silent());

    BOOST_REQUIRE(capture.write("data_out/release_reused_slot.png"));
    v3d::image::reader::Png png(headless.logger);
    boost::shared_ptr<v3d::image::Image> picture = png.read("data_out/release_reused_slot.png");
    BOOST_REQUIRE(picture);
    const unsigned char* pixels = picture->data();
    for (uint32_t texel = 0; texel < width * height; texel++) {
        const unsigned char* at = pixels + static_cast<std::size_t>(texel) * 4;
        if (at[0] != 0x00 || at[1] != 0xFF || at[2] != 0x00 || at[3] != 0xFF) {
            BOOST_ERROR("texel " << texel << " is not the texture now in the slot");
            return;
        }
    }
}

/**
 * The white texture is shared by every untextured quad, so releasing it is refused - and a
 * caller releasing what depthTexture() gave back for a target with nothing to sample is
 * releasing it.
 **/
BOOST_AUTO_TEST_CASE(the_white_texture_is_not_released) {
    v3d::test::Headless headless(colourFormat, width, height);

    TextureHandle white = headless.context->quads()->white();

    BOOST_CHECK(!headless.context->quads()->release(white));
    BOOST_CHECK(headless.context->resources()->texture(white) != nullptr);
}

BOOST_AUTO_TEST_SUITE_END()
