/**
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 **/

#include <api/image/Image.h>
#include <api/image/reader/Png.h>
#include <api/render/realtime/Frame.h>
#include <api/render/realtime/Pass.h>
#include <api/render/realtime/canvas/World.h>
#include <api/render/realtime/vulkan/frame/Capture.h>
#include <api/render/realtime/vulkan/frame/Recorder.h>
#include <api/render/realtime/vulkan/frame/RenderTarget.h>
#include <api/render/realtime/vulkan/renderer/World.h>

#include <cstddef>
#include <cstdint>
#include <string>
#include <vector>

#include <boost/make_shared.hpp>
#include <boost/test/unit_test.hpp>

#include <glm/mat4x4.hpp>
#include <glm/vec3.hpp>
#include <glm/vec4.hpp>

#include "Headless.h"

using v3d::render::realtime::Frame;
using v3d::render::realtime::Pass;
using v3d::render::realtime::vulkan::frame::Capture;
using v3d::render::realtime::vulkan::frame::Recorder;
using v3d::render::realtime::vulkan::frame::RenderTarget;
using v3d::render::realtime::vulkan::renderer::World;

namespace canvas = v3d::render::realtime::canvas;

namespace {

const VkFormat colourFormat = VK_FORMAT_R8G8B8A8_UNORM;
const uint32_t width = 32;
const uint32_t height = 16;

/**
 * A projection measuring the world in pixels of the target, as the depth case's does, so a
 * quad's corners land on pixel boundaries and every pixel is wholly in or out of it.
 **/
glm::mat4x4 pixels() {
    glm::mat4x4 projection(1.0F);
    projection[0][0] = 2.0F / static_cast<float>(width);
    projection[1][1] = 2.0F / static_cast<float>(height);
    projection[3][0] = -1.0F;
    projection[3][1] = -1.0F;
    return projection;
}

canvas::World::Corners rect(float minX, float maxX) {
    return canvas::World::Corners{
        glm::vec3(minX, 0.0F, 0.5F),
        glm::vec3(maxX, 0.0F, 0.5F),
        glm::vec3(maxX, static_cast<float>(height), 0.5F),
        glm::vec3(minX, static_cast<float>(height), 0.5F)
    };
}

/**
 * Clear to a colour every channel of which is a whole number of 8-bit steps, draw a canvas over
 * it with a blend, and read the picture back.
 *
 * @param tints one submission of the canvas for each, in order, all in the one pass
 **/
boost::shared_ptr<v3d::image::Image> blended(v3d::test::Headless* headless, const canvas::World& canvas,
    World::Blend blend, const std::string& name, const std::vector<glm::vec4>& tints = {glm::vec4(1.0F)}) {
    const boost::shared_ptr<RenderTarget> target = boost::make_shared<RenderTarget>(
        headless->device, headless->context->ring(), width, height, colourFormat, false);

    Frame frame;
    const boost::shared_ptr<Pass> pass = frame.pass("world");
    pass->target(target);
    pass->clearColour(glm::vec4(0.2F, 0.4F, 0.0F, 1.0F));
    pass->camera(glm::mat4x4(1.0F), pixels());
    for (const glm::vec4& tint : tints) {
        headless->context->worldQuads()->submit(canvas, pass.get(), 0, blend, tint);
    }

    VkCommandBuffer commands = headless->context->ring()->begin();
    Recorder::record(commands, frame, Recorder::Target(), *headless->context->resources(),
        headless->context->frameUniforms().get());

    Capture capture(headless->device, headless->context->ring(), headless->logger);
    Capture::Source source;
    source.image = target->image();
    source.extent = target->extent();
    source.format = target->format();
    source.layout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
    capture.record(commands, source);
    headless->submitAndWait(commands);

    const std::string path = "data_out/" + name + ".png";
    if (!capture.write(path)) {
        return boost::shared_ptr<v3d::image::Image>();
    }
    v3d::image::reader::Png png(headless->logger);
    return png.read(path);
}

/**
 * @return the texel at a column of the picture, in its middle row
 **/
glm::ivec4 at(v3d::image::Image* picture, uint32_t x) {
    const unsigned char* texel = picture->data() + (((static_cast<std::size_t>(height / 2) * width) + x) * 4);
    return glm::ivec4(texel[0], texel[1], texel[2], texel[3]);
}

BOOST_AUTO_TEST_SUITE(world_blend_test)

/**
 * An additive quad adds its colour, scaled by its alpha, to the colour already there, and leaves
 * the alpha unchanged. An alpha-blended quad covers the clear instead. The sums land on whole
 * 8-bit steps, well away from any rounding boundary, so the specification determines every
 * byte.
 *
 * The clear is 51 and 102 on a 255 scale. A quad of 0.4, 0.4 and 0.8 adds 102, 102 and 204 at
 * full alpha and half that at half alpha, so every sum is a multiple of 51.
 **/
BOOST_AUTO_TEST_CASE(an_additive_quad_adds_to_what_is_there) {
    v3d::test::Headless headless(colourFormat, width, height);

    canvas::World canvas;
    canvas.quad(rect(0.0F, 16.0F), glm::vec4(0.4F, 0.4F, 0.8F, 1.0F));
    canvas.quad(rect(16.0F, 32.0F), glm::vec4(0.4F, 0.4F, 0.8F, 0.5F));

    const boost::shared_ptr<v3d::image::Image> added = blended(&headless, canvas, World::Blend::Additive, "world_additive");
    BOOST_CHECK(headless.silent());
    BOOST_REQUIRE(added);
    BOOST_TEST((at(added.get(), 8) == glm::ivec4(153, 204, 204, 255)));
    BOOST_TEST((at(added.get(), 24) == glm::ivec4(102, 153, 102, 255)));

    // the same canvas blended over covers the clear with its own colour at full alpha
    const boost::shared_ptr<v3d::image::Image> over = blended(&headless, canvas, World::Blend::Alpha, "world_over");
    BOOST_CHECK(headless.silent());
    BOOST_REQUIRE(over);
    BOOST_TEST((at(over.get(), 8) == glm::ivec4(102, 102, 204, 255)));
}

/**
 * A submission's tint multiplies the colour of every quad in the canvas as it is drawn, so one
 * canvas built once is drawn in a different light without being built again. The tint scales a
 * quad's own colour rather than replacing it. Every product is a whole 8-bit step.
 **/
BOOST_AUTO_TEST_CASE(a_tint_lights_a_canvas_as_it_is_drawn) {
    v3d::test::Headless headless(colourFormat, width, height);

    canvas::World canvas;
    canvas.quad(rect(0.0F, 16.0F), glm::vec4(1.0F));
    canvas.quad(rect(16.0F, 32.0F), glm::vec4(0.5F, 0.5F, 0.5F, 1.0F));

    const boost::shared_ptr<v3d::image::Image> blue = blended(&headless, canvas, World::Blend::Alpha, "world_tint_blue",
        {glm::vec4(0.4F, 0.4F, 0.8F, 1.0F)});
    BOOST_CHECK(headless.silent());
    BOOST_REQUIRE(blue);
    BOOST_TEST((glm::ivec3(at(blue.get(), 8)) == glm::ivec3(102, 102, 204)));
    BOOST_TEST((glm::ivec3(at(blue.get(), 24)) == glm::ivec3(51, 51, 102)));

    // the same canvas, untouched, in another light
    const boost::shared_ptr<v3d::image::Image> red = blended(&headless, canvas, World::Blend::Alpha, "world_tint_red",
        {glm::vec4(0.8F, 0.4F, 0.4F, 1.0F)});
    BOOST_CHECK(headless.silent());
    BOOST_REQUIRE(red);
    BOOST_TEST((glm::ivec3(at(red.get(), 8)) == glm::ivec3(204, 102, 102)));
    BOOST_TEST((glm::ivec3(at(red.get(), 24)) == glm::ivec3(102, 51, 51)));
}

/**
 * Each submission carries its own tint, so a canvas submitted twice in one pass is drawn in two
 * lights. An opaque red submission is covered by a blue one at 0.4 alpha. Were the two to share
 * one tint, the picture would be all red or blue over the clear, never the mix.
 **/
BOOST_AUTO_TEST_CASE(two_submissions_of_a_canvas_keep_their_own_tints) {
    v3d::test::Headless headless(colourFormat, width, height);

    canvas::World canvas;
    canvas.quad(rect(0.0F, 32.0F), glm::vec4(1.0F));

    const boost::shared_ptr<v3d::image::Image> mixed = blended(&headless, canvas, World::Blend::Alpha, "world_tint_twice",
        {glm::vec4(1.0F, 0.0F, 0.0F, 1.0F), glm::vec4(0.0F, 0.0F, 1.0F, 0.4F)});
    BOOST_CHECK(headless.silent());
    BOOST_REQUIRE(mixed);
    BOOST_TEST((glm::ivec3(at(mixed.get(), 8)) == glm::ivec3(153, 0, 102)));
}

BOOST_AUTO_TEST_SUITE_END()

};  // namespace
