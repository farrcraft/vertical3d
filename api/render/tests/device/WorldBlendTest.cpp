/**
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 **/

#include <api/image/Image.h>
#include <api/image/reader/Png.h>
#include <api/render/realtime/Frame.h>
#include <api/render/realtime/Pass.h>
#include <api/render/realtime/WorldCanvas.h>
#include <api/render/realtime/vulkan/frame/Capture.h>
#include <api/render/realtime/vulkan/frame/Recorder.h>
#include <api/render/realtime/vulkan/frame/RenderTarget.h>
#include <api/render/realtime/vulkan/renderer/World.h>

#include <cstddef>
#include <string>

#include <boost/make_shared.hpp>
#include <boost/test/unit_test.hpp>

#include <glm/mat4x4.hpp>
#include <glm/vec3.hpp>
#include <glm/vec4.hpp>

#include "Headless.h"

using v3d::render::realtime::Frame;
using v3d::render::realtime::Pass;
using v3d::render::realtime::WorldCanvas;
using v3d::render::realtime::vulkan::frame::Capture;
using v3d::render::realtime::vulkan::frame::Recorder;
using v3d::render::realtime::vulkan::frame::RenderTarget;
using v3d::render::realtime::vulkan::renderer::World;

namespace {

const VkFormat colourFormat = VK_FORMAT_R8G8B8A8_UNORM;
const uint32_t width = 32;
const uint32_t height = 16;

/**
 * A projection measuring the world in pixels of the target, as the depth case's does, so a
 * quad's corners land on pixel boundaries and every pixel is wholly in or out of it.
 **/
glm::mat4x4 pixels() {
    glm::mat4x4 projection(1.0f);
    projection[0][0] = 2.0f / static_cast<float>(width);
    projection[1][1] = 2.0f / static_cast<float>(height);
    projection[3][0] = -1.0f;
    projection[3][1] = -1.0f;
    return projection;
}

WorldCanvas::Corners rect(float minX, float maxX) {
    return WorldCanvas::Corners{
        glm::vec3(minX, 0.0f, 0.5f),
        glm::vec3(maxX, 0.0f, 0.5f),
        glm::vec3(maxX, static_cast<float>(height), 0.5f),
        glm::vec3(minX, static_cast<float>(height), 0.5f)
    };
}

/**
 * Clear to a colour every channel of which is a whole number of 8-bit steps, draw a canvas over
 * it with a blend, and read the picture back.
 **/
boost::shared_ptr<v3d::image::Image> blended(v3d::test::Headless* headless, const WorldCanvas& canvas,
    World::Blend blend, const std::string& name) {
    boost::shared_ptr<RenderTarget> target = boost::make_shared<RenderTarget>(
        headless->device, headless->context->ring(), width, height, colourFormat, false);

    Frame frame(headless->context);
    boost::shared_ptr<Pass> pass = frame.pass("world");
    pass->target(target);
    pass->clearColour(glm::vec4(0.2f, 0.4f, 0.0f, 1.0f));
    pass->camera(glm::mat4x4(1.0f), pixels());
    headless->context->worldQuads()->submit(canvas, pass.get(), 0, blend);

    VkCommandBuffer commands = headless->context->ring()->begin();
    Recorder::record(commands, frame, Recorder::Target(), *headless->context->resources(),
        headless->context->frameUniforms().get());

    Capture capture(headless->device, headless->logger);
    Capture::Source source;
    source.image = target->image();
    source.extent = target->extent();
    source.format = target->format();
    source.layout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
    capture.record(commands, source);
    headless->submitAndWait(commands);
    headless->context->worldQuads()->endFrame();

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
    const unsigned char* texel = picture->data() + (static_cast<std::size_t>(height / 2) * width + x) * 4;
    return glm::ivec4(texel[0], texel[1], texel[2], texel[3]);
}

};  // namespace

BOOST_AUTO_TEST_SUITE(world_blend_test)

/**
 * An additive quad adds its colour, scaled by its alpha, to what is there and leaves the alpha
 * as it was, where the alpha pipeline covers it - and the sums land on whole 8-bit steps well
 * away from any rounding boundary, so the specification determines every byte (ADR-0054).
 *
 * The clear is 51 and 102 on a 255 scale. A quad of 0.4, 0.4 and 0.8 adds 102, 102 and 204 at
 * full alpha and half that at half alpha, so every sum is a multiple of 51.
 **/
BOOST_AUTO_TEST_CASE(an_additive_quad_adds_to_what_is_there) {
    v3d::test::Headless headless(colourFormat, width, height);

    WorldCanvas canvas;
    canvas.quad(rect(0.0f, 16.0f), glm::vec4(0.4f, 0.4f, 0.8f, 1.0f));
    canvas.quad(rect(16.0f, 32.0f), glm::vec4(0.4f, 0.4f, 0.8f, 0.5f));

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

BOOST_AUTO_TEST_SUITE_END()
