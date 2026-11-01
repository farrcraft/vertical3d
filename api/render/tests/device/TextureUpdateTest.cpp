/**
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 **/

#include <api/image/Image.h>
#include <api/image/reader/Png.h>
#include <api/render/realtime/DeviceContext.h>
#include <api/render/realtime/Frame.h>
#include <api/render/realtime/Handle.h>
#include <api/render/realtime/Pass.h>
#include <api/render/realtime/Recorder.h>
#include <api/render/realtime/RenderTarget.h>
#include <api/render/realtime/Textures.h>
#include <api/render/realtime/canvas/Canvas.h>
#include <api/render/realtime/vulkan/frame/Capture.h>
#include <api/render/realtime/vulkan/frame/Ring.h>
#include <api/render/realtime/vulkan/memory/TextureFactory.h>

#include <vulkan/vulkan.h>

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
using v3d::render::realtime::vulkan::frame::Capture;
using v3d::render::realtime::vulkan::memory::TextureFactory;

const VkFormat kColourFormat = VK_FORMAT_R8G8B8A8_UNORM;
const uint32_t kSize = 16;

/**
 * Texels of one colour with no blue, four channels a texel.
 *
 * @param colour its red and green
 **/
std::vector<unsigned char> flat(uint32_t texels, const glm::uvec2& colour) {
    std::vector<unsigned char> pixels;
    for (uint32_t texel = 0; texel < texels; texel++) {
        pixels.insert(pixels.end(), {static_cast<unsigned char>(colour.x), static_cast<unsigned char>(colour.y), 0x00, 0xFF});
    }
    return pixels;
}

/**
 * Record a frame that covers the target with a texture texel for texel, copy the target out,
 * and submit it without waiting.
 **/
void draw(v3d::test::Headless* headless, const v3d::render::realtime::RenderTarget& target, const TextureHandle& texture, Capture* capture) {
    v3d::render::realtime::canvas::Canvas canvas;
    canvas.resize(glm::uvec2(kSize, kSize));
    canvas.clear();
    canvas.rect(glm::vec2(0.0F), glm::vec2(static_cast<float>(kSize)), glm::vec2(0.0F), glm::vec2(1.0F), glm::vec4(1.0F), texture);

    v3d::render::realtime::Frame frame;
    const boost::shared_ptr<v3d::render::realtime::Pass> pass = frame.pass("colour");
    pass->clearColour(glm::vec4(0.0F, 0.0F, 0.0F, 1.0F));
    headless->context->quads()->submit(canvas, pass.get());

    VkCommandBuffer commands = headless->context->ring()->begin();
    v3d::render::realtime::Recorder::Target described;
    described.image = target.image();
    described.view = target.view();
    described.extent = target.extent();
    described.finalLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
    v3d::render::realtime::Recorder::record(commands, frame, described, *headless->context->resources(), headless->context->frameUniforms().get());

    Capture::Source source;
    source.image = target.image();
    source.extent = target.extent();
    source.format = target.format();
    source.layout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
    capture->record(commands, source);
    headless->submit(commands);
}

/**
 * @return how many texels of a capture written to a png are not the colour expected of them
 **/
template <typename Expected>
int wrong(v3d::test::Headless* headless, Capture* capture, const std::string& path, Expected expected) {
    BOOST_REQUIRE(capture->write(path));
    v3d::image::reader::Png png(headless->logger);
    const boost::shared_ptr<v3d::image::Image> picture = png.read(path);
    BOOST_REQUIRE(picture);
    int count = 0;
    for (uint32_t y = 0; y < kSize; y++) {
        for (uint32_t x = 0; x < kSize; x++) {
            const unsigned char* at = picture->data() + (((static_cast<std::size_t>(y) * kSize) + x) * 4);
            const glm::uvec2 colour = expected(x);
            count += at[0] != colour.x || at[1] != colour.y || at[2] != 0x00 || at[3] != 0xFF ? 1 : 0;
        }
    }
    return count;
}

BOOST_AUTO_TEST_SUITE(texture_update_test)

/**
 * A region updated while a frame drawing the texture is in flight leaves that frame drawing
 * what the texture held, and the next frame draws the new region beside the rest it kept. The
 * device orders the copy after the earlier frame's reads, which the validation layer checks.
 **/
BOOST_AUTO_TEST_CASE(a_region_updated_while_the_texture_is_drawn) {
    v3d::test::Headless headless(kColourFormat, kSize, kSize);
    const v3d::render::realtime::RenderTarget target(*headless.context, { .width = kSize, .height = kSize, .colour = kColourFormat });
    const std::vector<unsigned char> red = flat(kSize * kSize, glm::uvec2(0xFF, 0x00));
    const TextureHandle texture = headless.context->textures()->texture(red.data(), kSize, kSize, 4);
    const uint32_t slot = headless.context->textures()->slot(texture);

    Capture before(headless.device, headless.context->ring(), headless.logger);
    draw(&headless, target, texture, &before);

    const std::vector<unsigned char> green = flat(kSize / 2 * kSize, glm::uvec2(0x00, 0xFF));
    BOOST_REQUIRE(headless.context->textures()->update(texture, glm::uvec2(0, 0),
        TextureFactory::Pixels{ .data = green.data(), .width = kSize / 2, .height = kSize, .channels = 4 }));
    BOOST_CHECK_EQUAL(headless.context->textures()->slot(texture), slot);

    Capture after(headless.device, headless.context->ring(), headless.logger);
    draw(&headless, target, texture, &after);
    for (uint32_t turn = 0; turn <= headless.context->ring()->framesInFlight(); turn++) {
        headless.submit(headless.context->ring()->begin());
    }
    headless.context->ring()->waitIdle();

    BOOST_CHECK(headless.silent());
    BOOST_CHECK_EQUAL(wrong(&headless, &before, "data_out/texture_update_before.png", [](uint32_t) { return glm::uvec2(0xFF, 0x00); }), 0);
    BOOST_CHECK_EQUAL(wrong(&headless, &after, "data_out/texture_update_after.png",
        [](uint32_t x) { return x < kSize / 2 ? glm::uvec2(0x00, 0xFF) : glm::uvec2(0xFF, 0x00); }), 0);
}

/**
 * Nothing is copied into the white texture, a render target's image, a released texture, a
 * region outside the texture, or a texture with other channels. A coverage mask takes one
 * channel.
 **/
BOOST_AUTO_TEST_CASE(an_update_that_does_not_fit_is_refused) {
    v3d::test::Headless headless(kColourFormat, kSize, kSize);
    const boost::shared_ptr<v3d::render::realtime::Textures> textures = headless.context->textures();
    const std::vector<unsigned char> colour = flat(4 * 4, glm::uvec2(0xFF, 0xFF));
    const std::vector<unsigned char> mask(16, 0x80);
    const TextureFactory::Pixels four{ .data = colour.data(), .width = 4, .height = 4, .channels = 4 };
    const TextureFactory::Pixels one{ .data = mask.data(), .width = 4, .height = 4, .channels = 1 };

    const TextureHandle texture = textures->texture(colour.data(), 8, 2, 4);
    BOOST_CHECK(!textures->update(textures->white(), glm::uvec2(0, 0), TextureFactory::Pixels{ .data = colour.data(), .width = 1, .height = 1 }));
    BOOST_CHECK(!textures->update(texture, glm::uvec2(0, 0), four));
    BOOST_CHECK(!textures->update(texture, glm::uvec2(6, 0), TextureFactory::Pixels{ .data = colour.data(), .width = 4, .height = 1 }));
    BOOST_CHECK(textures->update(texture, glm::uvec2(4, 0), TextureFactory::Pixels{ .data = colour.data(), .width = 4, .height = 2 }));
    BOOST_CHECK(!textures->update(texture, glm::uvec2(0, 0), TextureFactory::Pixels{ .data = mask.data(), .width = 2, .height = 2, .channels = 1 }));

    const TextureHandle coverage = textures->texture(mask.data(), 4, 4, 1);
    BOOST_CHECK(textures->update(coverage, glm::uvec2(0, 0), one));
    BOOST_CHECK(!textures->update(coverage, glm::uvec2(0, 0), four));

    const v3d::render::realtime::RenderTarget target(*headless.context, { .width = 4, .height = 4, .colour = kColourFormat });
    BOOST_CHECK(!textures->update(textures->texture(target), glm::uvec2(0, 0), four));

    BOOST_REQUIRE(textures->release(texture));
    BOOST_CHECK(!textures->update(texture, glm::uvec2(0, 0), TextureFactory::Pixels{ .data = colour.data(), .width = 1, .height = 1 }));

    headless.submit(headless.context->ring()->begin());
    headless.context->ring()->waitIdle();
    BOOST_CHECK(headless.silent());
}

BOOST_AUTO_TEST_SUITE_END()

};  // namespace
