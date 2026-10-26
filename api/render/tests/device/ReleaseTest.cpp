/**
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 **/

#include <api/image/Image.h>
#include <api/image/reader/Png.h>
#include <api/render/realtime/DeviceContext.h>
#include <api/render/realtime/Handle.h>
#include <api/render/realtime/canvas/Canvas.h>
#include <api/render/realtime/Frame.h>
#include <api/render/realtime/Pass.h>
#include <api/render/realtime/vulkan/frame/Capture.h>
#include <api/render/realtime/vulkan/frame/DepthBuffer.h>
#include <api/render/realtime/Recorder.h>
#include <api/render/realtime/RenderTarget.h>
#include <api/render/realtime/vulkan/frame/StreamRing.h>
#include <api/render/realtime/Resources.h>

#include <cstddef>
#include <cstdint>
#include <string>
#include <vector>

#include <boost/make_shared.hpp>
#include <boost/test/unit_test.hpp>

#include "Headless.h"

using v3d::render::realtime::canvas::Canvas;
using v3d::render::realtime::Frame;
using v3d::render::realtime::Pass;
using v3d::render::realtime::TextureHandle;
using v3d::render::realtime::vulkan::frame::Capture;
using v3d::render::realtime::Recorder;
using v3d::render::realtime::RenderTarget;

namespace {

const VkFormat colourFormat = VK_FORMAT_R8G8B8A8_UNORM;
const uint32_t width = 16;
const uint32_t height = 16;

/**
 * A texture that is one flat colour, so that wherever and however it is sampled what reaches
 * the target is that colour exactly, on any conformant driver.
 **/
TextureHandle flat(v3d::test::Headless* headless, unsigned char red, unsigned char green, unsigned char blue) {
    std::vector<unsigned char> pixels;
    for (int texel = 0; texel < 4 * 4; texel++) {
        pixels.insert(pixels.end(), {red, green, blue, 0xFF});
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
    canvas.resize(glm::uvec2(width, height));
    canvas.clear();
    canvas.rect(glm::vec2(0.0F, 0.0F), glm::vec2(static_cast<float>(width), static_cast<float>(height)), glm::vec2(0.0F, 0.0F), glm::vec2(1.0F, 1.0F),
        glm::vec4(1.0F), texture);

    Frame frame;
    const boost::shared_ptr<Pass> pass = frame.pass("colour");
    pass->clearColour(glm::vec4(0.0F, 0.0F, 0.0F, 1.0F));
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
    const boost::shared_ptr<RenderTarget> target = boost::make_shared<RenderTarget>(headless.device, headless.context->ring(), width, height, colourFormat);

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
 * A sprite whose texture is released after the sprite is queued, and before the frame is
 * begun, is drawn with that texture by that frame. The queued item names the texture's material
 * by handle, and the frame resolves it when it is recorded.
 **/
BOOST_AUTO_TEST_CASE(a_sprite_whose_texture_is_released_after_it_is_queued_draws_it) {
    v3d::test::Headless headless(colourFormat, width, height);
    const boost::shared_ptr<RenderTarget> target = boost::make_shared<RenderTarget>(headless.device, headless.context->ring(), width, height, colourFormat);

    // one frame submitted first, so the ring has begun a frame before the release
    idle(&headless);

    const TextureHandle texture = flat(&headless, 0xFF, 0x00, 0xFF);
    Canvas canvas;
    canvas.resize(glm::uvec2(width, height));
    canvas.clear();
    canvas.rect(glm::vec2(0.0F, 0.0F), glm::vec2(static_cast<float>(width), static_cast<float>(height)), glm::vec2(0.0F, 0.0F), glm::vec2(1.0F, 1.0F),
        glm::vec4(1.0F), texture);

    Frame frame;
    const boost::shared_ptr<Pass> pass = frame.pass("colour");
    pass->clearColour(glm::vec4(0.0F, 0.0F, 0.0F, 1.0F));
    headless.context->quads()->submit(canvas, pass.get());

    BOOST_CHECK(headless.context->textures()->release(texture));

    VkCommandBuffer commands = headless.context->ring()->begin();
    Recorder::Target described;
    described.image = target->image();
    described.view = target->view();
    described.extent = target->extent();
    described.finalLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
    Recorder::record(commands, frame, described, *headless.context->resources(), headless.context->frameUniforms().get());

    Capture capture(headless.device, headless.context->ring(), headless.logger);
    Capture::Source source;
    source.image = target->image();
    source.extent = target->extent();
    source.format = target->format();
    source.layout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
    capture.record(commands, source);
    headless.submit(commands);

    for (uint32_t turn = 0; turn <= headless.context->ring()->framesInFlight(); turn++) {
        idle(&headless);
    }
    headless.context->ring()->waitIdle();

    BOOST_CHECK(headless.silent());

    BOOST_REQUIRE(capture.write("data_out/release_sprite_after_queueing.png"));
    v3d::image::reader::Png png(headless.logger);
    const boost::shared_ptr<v3d::image::Image> picture = png.read("data_out/release_sprite_after_queueing.png");
    BOOST_REQUIRE(picture);
    const unsigned char* pixels = picture->data();
    for (uint32_t texel = 0; texel < width * height; texel++) {
        const unsigned char* at = pixels + (static_cast<std::size_t>(texel) * 4);
        if (at[0] != 0xFF || at[1] != 0x00 || at[2] != 0xFF || at[3] != 0xFF) {
            BOOST_ERROR("texel " << texel << " is not the released sprite's texture");
            return;
        }
    }
}

/**
 * A material released goes on resolving until the frame queued at its release is submitted, and
 * a texture released resolves to nothing at once. Each is held until that frame has finished,
 * and a second release of either finds nothing to release.
 **/
BOOST_AUTO_TEST_CASE(a_released_handle_is_held_until_its_frame_finishes) {
    v3d::test::Headless headless(colourFormat, width, height);
    const boost::shared_ptr<v3d::render::realtime::Resources> resources = headless.context->resources();

    idle(&headless);
    const std::size_t before = resources->textureCount();
    const TextureHandle texture = flat(&headless, 0xFF, 0x00, 0x00);
    const v3d::render::realtime::MaterialHandle material = resources->add(v3d::render::realtime::Material());

    BOOST_CHECK(resources->release(material));
    BOOST_CHECK(!resources->release(material));
    BOOST_CHECK(resources->release(texture));
    BOOST_CHECK(!resources->release(texture));

    // the frame queued at the release is recorded with the material, and nothing new can name
    // the texture
    BOOST_CHECK(resources->texture(texture) == nullptr);
    BOOST_CHECK_EQUAL(resources->textureCount(), before + 1);
    VkCommandBuffer commands = headless.context->ring()->begin();
    BOOST_CHECK(resources->material(material) != nullptr);
    headless.submit(commands);
    BOOST_CHECK(resources->material(material) == nullptr);
    BOOST_CHECK_EQUAL(resources->textureCount(), before + 1);

    for (uint32_t turn = 0; turn < headless.context->ring()->framesInFlight(); turn++) {
        idle(&headless);
    }
    headless.context->ring()->waitIdle();
    BOOST_CHECK_EQUAL(resources->textureCount(), before);
    BOOST_CHECK(!resources->release(material));
    BOOST_CHECK(headless.silent());
}

/**
 * A texture registered once a released one's frames have finished reuses the released slot, and
 * draws as itself rather than as the texture that was in the slot before. A material looked up
 * by slot alone would hand back the old descriptor set, naming an image that has been destroyed.
 **/
BOOST_AUTO_TEST_CASE(a_reused_slot_draws_its_new_texture) {
    v3d::test::Headless headless(colourFormat, width, height);
    const boost::shared_ptr<RenderTarget> target = boost::make_shared<RenderTarget>(headless.device, headless.context->ring(), width, height, colourFormat);

    TextureHandle red = flat(&headless, 0xFF, 0x00, 0x00);
    draw(&headless, target, red, nullptr);
    BOOST_REQUIRE(headless.context->textures()->release(red));
    for (uint32_t turn = 0; turn <= headless.context->ring()->framesInFlight(); turn++) {
        idle(&headless);
    }

    TextureHandle green = flat(&headless, 0x00, 0xFF, 0x00);
    BOOST_REQUIRE_EQUAL(green.id(), red.id());

    Capture capture(headless.device, headless.context->ring(), headless.logger);
    draw(&headless, target, green, &capture);
    headless.context->ring()->waitIdle();

    BOOST_CHECK(headless.silent());

    BOOST_REQUIRE(capture.write("data_out/release_reused_slot.png"));
    v3d::image::reader::Png png(headless.logger);
    const boost::shared_ptr<v3d::image::Image> picture = png.read("data_out/release_reused_slot.png");
    BOOST_REQUIRE(picture);
    const unsigned char* pixels = picture->data();
    for (uint32_t texel = 0; texel < width * height; texel++) {
        const unsigned char* at = pixels + (static_cast<std::size_t>(texel) * 4);
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
    const boost::shared_ptr<RenderTarget> target = boost::make_shared<RenderTarget>(headless.device, headless.context->ring(), width, height, colourFormat, true);

    draw(&headless, target, flat(&headless, 0xFF, 0x00, 0x00), nullptr);
    target->recreate(width / 2, height / 2);

    for (int frame = 0; frame < 3; frame++) {
        idle(&headless);
    }
    headless.context->ring()->waitIdle();

    BOOST_CHECK(headless.silent());
}

/**
 * A texture registered from a target shares the target's image. A resize therefore leaves the
 * registered texture naming the old image rather than one that has been destroyed, and a
 * frame that still samples it draws.
 **/
BOOST_AUTO_TEST_CASE(a_registered_target_keeps_its_image_through_a_resize) {
    v3d::test::Headless headless(colourFormat, width, height);
    const boost::shared_ptr<RenderTarget> source = boost::make_shared<RenderTarget>(headless.device, headless.context->ring(), width, height, colourFormat);
    const boost::shared_ptr<RenderTarget> into = boost::make_shared<RenderTarget>(headless.device, headless.context->ring(), width, height, colourFormat);

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
    const v3d::test::Headless headless(colourFormat, width, height);

    TextureHandle white = headless.context->textures()->white();

    BOOST_CHECK(!headless.context->textures()->release(white));
    BOOST_CHECK(headless.context->resources()->texture(white) != nullptr);
}

/**
 * A capture of a larger image replaces the readback buffer and retires the old one through the
 * ring, because the copy recorded into an earlier frame may still be writing into it. The frames
 * before the case run out first, so the only retirement left is the one the capture makes.
 **/
BOOST_AUTO_TEST_CASE(a_capture_retires_the_readback_it_outgrows) {
    v3d::test::Headless headless(colourFormat, width, height);
    const boost::shared_ptr<RenderTarget> small = boost::make_shared<RenderTarget>(headless.device, headless.context->ring(), width, height, colourFormat);
    const boost::shared_ptr<RenderTarget> large = boost::make_shared<RenderTarget>(headless.device, headless.context->ring(), width * 2, height * 2,
        colourFormat);
    const TextureHandle texture = flat(&headless, 0x00, 0xFF, 0x00);
    Capture capture(headless.device, headless.context->ring(), headless.logger);

    draw(&headless, small, texture, &capture);
    for (uint32_t turn = 0; turn <= headless.context->ring()->framesInFlight(); turn++) {
        idle(&headless);
    }
    BOOST_CHECK_EQUAL(headless.context->ring()->retiring(), 0U);
    draw(&headless, large, texture, &capture);
    BOOST_CHECK_EQUAL(headless.context->ring()->retiring(), 1U);

    headless.context->ring()->waitIdle();
    BOOST_CHECK(capture.write("data_out/release_capture_outgrown.png"));
    BOOST_CHECK(headless.silent());
}

/**
 * A context given a new extent replaces the depth buffer it allocated with one of that size, and
 * retires the old one through the ring, because a frame in flight may still test against it. The
 * same extent again keeps the buffer.
 **/
BOOST_AUTO_TEST_CASE(a_new_extent_retires_the_depth_buffer) {
    /** A context whose destination a case can describe, as WindowContext does after a resize. **/
    class Described final : public v3d::render::realtime::DeviceContext {
     public:
        using DeviceContext::DeviceContext;
        using DeviceContext::describe;
    };

    const v3d::test::Headless headless(colourFormat, width, height);
    const boost::shared_ptr<Described> context = boost::make_shared<Described>(headless.logger, headless.device, colourFormat,
        VkExtent2D{width, height});
    const boost::shared_ptr<v3d::render::realtime::vulkan::frame::DepthBuffer> first = context->depth();
    const std::size_t before = context->ring()->retiring();

    context->describe(colourFormat, VkExtent2D{width, height});
    BOOST_CHECK(context->depth() == first);
    BOOST_CHECK_EQUAL(context->ring()->retiring(), before);

    context->describe(colourFormat, VkExtent2D{width * 2, height * 2});
    BOOST_CHECK(context->depth() != first);
    BOOST_CHECK_EQUAL(context->depth()->extent().width, width * 2);
    BOOST_CHECK_EQUAL(context->depth()->extent().height, height * 2);
    BOOST_CHECK_EQUAL(context->ring()->retiring(), before + 1);
    BOOST_CHECK(headless.silent());
}

/**
 * A stream holds as many buffers as the busiest frame claimed, per frame in flight, however
 * many frames run. Nothing tells the stream that a frame ended, as with a renderer an app
 * builds itself. A buffer the content outgrows is replaced and retired, not added to.
 **/
BOOST_AUTO_TEST_CASE(a_stream_holds_what_one_frame_asked_for) {
    v3d::test::Headless headless(colourFormat, width, height);
    v3d::render::realtime::vulkan::frame::StreamRing stream(headless.device, headless.context->ring(), {.vertexBytes = 64, .indexBytes = 32});
    const std::size_t frames = headless.context->ring()->framesInFlight();

    for (int frame = 0; frame < 12; ++frame) {
        stream.claim({.vertexBytes = 16, .indexBytes = 8});
        stream.claim({.vertexBytes = 16, .indexBytes = 8});
        idle(&headless);
    }
    BOOST_CHECK_EQUAL(stream.held(), 2 * frames);

    for (std::size_t frame = 0; frame < frames * 2; ++frame) {
        const v3d::render::realtime::vulkan::frame::StreamRing::Geometry grown = stream.claim({.vertexBytes = 1000, .indexBytes = 8});
        BOOST_CHECK_GE(grown.vertices->size(), 1000U);
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
    const v3d::test::Headless headless(colourFormat, width, height);
    const boost::shared_ptr<RenderTarget> target = boost::make_shared<RenderTarget>(headless.device, headless.context->ring(),
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
    const v3d::test::Headless headless(colourFormat, width, height);
    v3d::render::realtime::vulkan::frame::StreamRing stream(headless.device, headless.context->ring(), {.vertexBytes = 64, .indexBytes = 32});

    for (int frame = 0; frame < 12; ++frame) {
        stream.claim({.vertexBytes = 16, .indexBytes = 8});
        stream.claim({.vertexBytes = 16, .indexBytes = 8});
        headless.context->ring()->skip();
    }
    BOOST_CHECK_EQUAL(stream.held(), 2U);
    BOOST_CHECK(headless.silent());
}

/**
 * A renderer claims before the frame begins. A frame begun and abandoned, because recording
 * threw, was never drawn, so the attempt that follows reuses its claims. That holds however
 * many times the same slot is begun again, so the slot holds one pair. The first claim on the
 * next slot then takes that slot's first pair.
 **/
BOOST_AUTO_TEST_CASE(a_frame_begun_again_restarts_its_claims) {
    v3d::test::Headless headless(colourFormat, width, height);
    v3d::render::realtime::vulkan::frame::StreamRing stream(headless.device, headless.context->ring(), {.vertexBytes = 64, .indexBytes = 32});

    stream.claim({.vertexBytes = 16, .indexBytes = 8});
    headless.context->ring()->begin();

    stream.claim({.vertexBytes = 16, .indexBytes = 8});
    headless.context->ring()->begin();

    stream.claim({.vertexBytes = 16, .indexBytes = 8});
    headless.submit(headless.context->ring()->begin());
    BOOST_CHECK_EQUAL(stream.held(), 1U);

    const v3d::render::realtime::vulkan::frame::StreamRing::Geometry next = stream.claim({.vertexBytes = 16, .indexBytes = 8});
    BOOST_CHECK(next.vertices != nullptr);
    BOOST_CHECK_EQUAL(stream.held(), 2U);
    headless.context->ring()->waitIdle();
    BOOST_CHECK(headless.silent());
}

/**
 * Claims made after a frame begins count in that slot. A claim made after the submit, before
 * the next frame begins, is in the next slot and starts from its front, whatever the slot
 * before claimed.
 **/
BOOST_AUTO_TEST_CASE(a_claim_on_the_next_slot_starts_from_its_front) {
    v3d::test::Headless headless(colourFormat, width, height);
    v3d::render::realtime::vulkan::frame::StreamRing stream(headless.device, headless.context->ring(), {.vertexBytes = 64, .indexBytes = 32});

    headless.context->ring()->begin();
    stream.claim({.vertexBytes = 16, .indexBytes = 8});

    VkCommandBuffer commands = headless.context->ring()->begin();
    stream.claim({.vertexBytes = 16, .indexBytes = 8});
    headless.submit(commands);

    const v3d::render::realtime::vulkan::frame::StreamRing::Geometry next = stream.claim({.vertexBytes = 16, .indexBytes = 8});
    BOOST_CHECK(next.vertices != nullptr);
    BOOST_CHECK_EQUAL(stream.held(), 2U);
    headless.context->ring()->waitIdle();
    BOOST_CHECK(headless.silent());
}

/**
 * A target's depth drawn on a canvas is sampled in the layout the recorder leaves it in, which
 * is read only for depth rather than for shaders. The layer reports the draw if any other
 * layout is used.
 **/
BOOST_AUTO_TEST_CASE(a_depth_texture_draws_on_a_canvas) {
    v3d::test::Headless headless(colourFormat, width, height);
    const boost::shared_ptr<RenderTarget> source = boost::make_shared<RenderTarget>(headless.device, headless.context->ring(), width, height,
        colourFormat, true, true);
    const boost::shared_ptr<RenderTarget> into = boost::make_shared<RenderTarget>(headless.device, headless.context->ring(), width, height, colourFormat);
    const TextureHandle depth = headless.context->textures()->depthTexture(*source);

    Canvas canvas;
    canvas.resize(glm::uvec2(width, height));
    canvas.clear();
    canvas.rect(glm::vec2(0.0F, 0.0F), glm::vec2(static_cast<float>(width), static_cast<float>(height)), glm::vec2(0.0F, 0.0F), glm::vec2(1.0F, 1.0F),
        glm::vec4(1.0F), depth);

    Frame frame;
    const boost::shared_ptr<Pass> drawn = frame.pass("colour");
    drawn->target(into);
    drawn->reads(source);
    drawn->clearColour(glm::vec4(0.0F, 0.0F, 0.0F, 1.0F));
    headless.context->quads()->submit(canvas, drawn.get());
    const boost::shared_ptr<Pass> cleared = frame.pass("depth");
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

};  // namespace
