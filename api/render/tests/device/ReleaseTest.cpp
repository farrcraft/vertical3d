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
#include <api/render/realtime/vulkan/frame/StreamRing.h>

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
 * the target is that colour exactly, on any conformant driver.
 **/
TextureHandle flat(v3d::test::Headless* headless, unsigned char r, unsigned char g, unsigned char b) {
    std::vector<unsigned char> pixels;
    for (int texel = 0; texel < 4 * 4; texel++) {
        pixels.insert(pixels.end(), {r, g, b, 0xFF});
    }
    return headless->context->textures()->texture(pixels.data(), 4, 4, 4);
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

    Frame frame;
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
 * under that frame. Destroying it at once is a use after free on the device, which only the
 * validation layer reports, so the case passes if the layer reports no errors.
 *
 * The layer treats a frame as finished only once the app waits on its fence, so the frame
 * counts as in flight however quickly the device drew it.
 **/
BOOST_AUTO_TEST_CASE(a_texture_released_in_flight_outlives_its_frame) {
    v3d::test::Headless headless(colourFormat, width, height);
    boost::shared_ptr<RenderTarget> target = boost::make_shared<RenderTarget>(headless.device, headless.context->ring(), width, height, colourFormat);

    TextureHandle texture = flat(&headless, 0xFF, 0x00, 0x00);
    draw(&headless, target, texture, nullptr);

    BOOST_CHECK(headless.context->textures()->release(texture));
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
    boost::shared_ptr<RenderTarget> target = boost::make_shared<RenderTarget>(headless.device, headless.context->ring(), width, height, colourFormat);

    TextureHandle red = flat(&headless, 0xFF, 0x00, 0x00);
    draw(&headless, target, red, nullptr);
    BOOST_REQUIRE(headless.context->textures()->release(red));

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
 * A target resized while a frame drawing into it is still in flight keeps its old images
 * until that frame has finished. Destroying them at once is the same use after free as a
 * texture released in flight, and is reported by the same layer.
 **/
BOOST_AUTO_TEST_CASE(a_target_resized_in_flight_outlives_its_frame) {
    v3d::test::Headless headless(colourFormat, width, height);
    boost::shared_ptr<RenderTarget> target = boost::make_shared<RenderTarget>(headless.device, headless.context->ring(), width, height, colourFormat, true);

    draw(&headless, target, flat(&headless, 0xFF, 0x00, 0x00), nullptr);
    target->recreate(width / 2, height / 2);

    for (int frame = 0; frame < 3; frame++) {
        idle(&headless);
    }
    headless.context->ring()->waitIdle();

    BOOST_CHECK(headless.silent());
}

/**
 * A texture registered from a target shares the target's image, so a resize leaves the
 * registered texture naming the old image rather than one that has been destroyed, and a
 * frame that still samples it draws.
 **/
BOOST_AUTO_TEST_CASE(a_registered_target_keeps_its_image_through_a_resize) {
    v3d::test::Headless headless(colourFormat, width, height);
    boost::shared_ptr<RenderTarget> source = boost::make_shared<RenderTarget>(headless.device, headless.context->ring(), width, height, colourFormat);
    boost::shared_ptr<RenderTarget> into = boost::make_shared<RenderTarget>(headless.device, headless.context->ring(), width, height, colourFormat);

    draw(&headless, source, flat(&headless, 0x00, 0x00, 0xFF), nullptr);
    const TextureHandle registered = headless.context->textures()->texture(*source);
    source->recreate(width / 2, height / 2);

    for (int frame = 0; frame < 3; frame++) {
        idle(&headless);
    }
    draw(&headless, into, registered, nullptr);
    headless.context->ring()->waitIdle();

    BOOST_CHECK(headless.silent());
}

/**
 * The white texture is shared by every untextured quad, so releasing it is refused. depthTexture()
 * returns the white texture for a target with no depth to sample, so a caller that releases
 * that handle is releasing the white texture.
 **/
BOOST_AUTO_TEST_CASE(the_white_texture_is_not_released) {
    v3d::test::Headless headless(colourFormat, width, height);

    TextureHandle white = headless.context->textures()->white();

    BOOST_CHECK(!headless.context->textures()->release(white));
    BOOST_CHECK(headless.context->resources()->texture(white) != nullptr);
}

/**
 * A stream holds as many buffers as the busiest frame claimed, per frame in flight, however
 * many frames run. Nothing tells the stream that a frame ended, as with a renderer an app
 * builds itself. A buffer the content outgrows is replaced and retired, not added to.
 **/
BOOST_AUTO_TEST_CASE(a_stream_holds_what_one_frame_asked_for) {
    v3d::test::Headless headless(colourFormat, width, height);
    v3d::render::realtime::vulkan::frame::StreamRing stream(headless.device, headless.context->ring(), 64, 32);
    const std::size_t frames = headless.context->ring()->framesInFlight();

    for (int frame = 0; frame < 12; ++frame) {
        stream.claim(16, 8);
        stream.claim(16, 8);
        idle(&headless);
    }
    BOOST_CHECK_EQUAL(stream.held(), 2 * frames);

    for (std::size_t frame = 0; frame < frames * 2; ++frame) {
        const v3d::render::realtime::vulkan::frame::StreamRing::Geometry grown = stream.claim(1000, 8);
        BOOST_CHECK_GE(grown.vertices->size(), 1000u);
        idle(&headless);
    }
    BOOST_CHECK_EQUAL(stream.held(), 2 * frames);
    BOOST_CHECK(headless.silent());
}

/**
 * Registering a target's image again gives back the handle it already has, so registering it
 * every frame does not grow the registry. Once that handle is released, registering it again
 * gives a new one.
 **/
BOOST_AUTO_TEST_CASE(a_target_registered_twice_has_one_handle) {
    v3d::test::Headless headless(colourFormat, width, height);
    boost::shared_ptr<RenderTarget> target = boost::make_shared<RenderTarget>(headless.device, headless.context->ring(),
        width, height, colourFormat, true, true);

    const TextureHandle first = headless.context->textures()->texture(*target);
    BOOST_CHECK(headless.context->textures()->texture(*target) == first);
    const TextureHandle depth = headless.context->textures()->depthTexture(*target);
    BOOST_CHECK(headless.context->textures()->depthTexture(*target) == depth);
    BOOST_CHECK(depth != first);

    BOOST_REQUIRE(headless.context->textures()->release(first));
    const TextureHandle again = headless.context->textures()->texture(*target);
    BOOST_CHECK(again != first);
    BOOST_CHECK(headless.context->resources()->texture(again) != nullptr);
    BOOST_CHECK(headless.silent());
}

/**
 * A frame that is skipped, because there was no image to draw into, never begins. The next
 * frame reuses what the skipped one claimed rather than claiming after it, so a run of
 * skipped frames holds no more than one frame's worth.
 **/
BOOST_AUTO_TEST_CASE(a_skipped_frame_gives_its_claims_back) {
    v3d::test::Headless headless(colourFormat, width, height);
    v3d::render::realtime::vulkan::frame::StreamRing stream(headless.device, headless.context->ring(), 64, 32);

    for (int frame = 0; frame < 12; ++frame) {
        stream.claim(16, 8);
        stream.claim(16, 8);
        headless.context->ring()->skip();
    }
    BOOST_CHECK_EQUAL(stream.held(), 2u);
    BOOST_CHECK(headless.silent());
}

/**
 * A target's depth drawn on a canvas is sampled in the layout the recorder leaves it in, which
 * is read only for depth rather than for shaders. The layer reports the draw if any other
 * layout is used.
 **/
BOOST_AUTO_TEST_CASE(a_depth_texture_draws_on_a_canvas) {
    v3d::test::Headless headless(colourFormat, width, height);
    boost::shared_ptr<RenderTarget> source = boost::make_shared<RenderTarget>(headless.device, headless.context->ring(), width, height,
        colourFormat, true, true);
    boost::shared_ptr<RenderTarget> into = boost::make_shared<RenderTarget>(headless.device, headless.context->ring(), width, height, colourFormat);
    const TextureHandle depth = headless.context->textures()->depthTexture(*source);

    Canvas canvas;
    canvas.resize(width, height);
    canvas.clear();
    canvas.rect(glm::vec2(0.0f, 0.0f), glm::vec2(static_cast<float>(width), static_cast<float>(height)), glm::vec2(0.0f, 0.0f), glm::vec2(1.0f, 1.0f),
        glm::vec4(1.0f), depth);

    Frame frame;
    boost::shared_ptr<Pass> drawn = frame.pass("colour");
    drawn->target(into);
    drawn->reads(source);
    drawn->clearColour(glm::vec4(0.0f, 0.0f, 0.0f, 1.0f));
    headless.context->quads()->submit(canvas, drawn.get());
    boost::shared_ptr<Pass> cleared = frame.pass("depth");
    cleared->target(source);
    cleared->depth(true);

    VkCommandBuffer commands = headless.context->ring()->begin();
    Recorder::record(commands, frame, Recorder::Target(), *headless.context->resources(),
        headless.context->frameUniforms().get());
    headless.submit(commands);
    headless.context->ring()->waitIdle();

    BOOST_CHECK(headless.silent());
}

BOOST_AUTO_TEST_SUITE_END()
