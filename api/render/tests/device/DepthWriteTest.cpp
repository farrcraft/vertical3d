/**
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 **/

#include <api/image/Image.h>
#include <api/image/reader/Png.h>
#include <api/render/realtime/Frame.h>
#include <api/render/realtime/canvas/Line.h>
#include <api/render/realtime/Pass.h>
#include <api/render/realtime/canvas/World.h>
#include <api/render/realtime/vulkan/frame/Capture.h>
#include <api/render/realtime/vulkan/frame/Recorder.h>
#include <api/render/realtime/vulkan/frame/RenderTarget.h>
#include <api/render/realtime/vulkan/renderer/Line.h>
#include <api/render/realtime/vulkan/renderer/World.h>

#include <cstddef>
#include <optional>
#include <string>

#include <boost/filesystem/operations.hpp>
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

namespace canvas = v3d::render::realtime::canvas;

namespace {

const VkFormat colourFormat = VK_FORMAT_R8G8B8A8_UNORM;
const uint32_t width = 64;
const uint32_t height = 32;

/**
 * A projection that measures the world in pixels of the target: x right over [0, width), y
 * down over [0, height), and z into the screen over [0, 1].
 **/
glm::mat4x4 pixels() {
    glm::mat4x4 projection(1.0f);
    projection[0][0] = 2.0f / static_cast<float>(width);
    projection[1][1] = 2.0f / static_cast<float>(height);
    projection[3][0] = -1.0f;
    projection[3][1] = -1.0f;
    return projection;
}

/**
 * What a case draws twice at the same depth.
 **/
enum class Drawn {
    Lines,
    WorldQuads
};

/**
 * A red shape and then a green one over the same pixels at the same depth, drawn into a depth
 * tested pass, and the colour of a pixel they both cover.
 *
 * The shapes are lines along one row, or world quads over a band of rows. Both shapes are one
 * draw of the same pipeline, so they cover the same pixels on any driver. Where the first shape
 * writes depth, the second fails the LESS test at the equal depth.
 *
 * @param write what the pass names for depth writing
 * @return "red", "green" or "neither", for the pixel at the middle of row 16
 **/
std::string drawCoincident(v3d::test::Headless* headless, Drawn drawn, std::optional<bool> write, const std::string& path) {
    boost::shared_ptr<RenderTarget> target = boost::make_shared<RenderTarget>(
        headless->device, headless->context->ring(), width, height, colourFormat, true);

    const glm::vec4 red(1.0f, 0.0f, 0.0f, 1.0f);
    const glm::vec4 green(0.0f, 1.0f, 0.0f, 1.0f);

    Frame frame;
    boost::shared_ptr<Pass> pass = frame.pass("coincident");
    pass->target(target);
    pass->clearColour(glm::vec4(0.0f, 0.0f, 0.0f, 1.0f));
    pass->depth(true);
    pass->depthWrite(write);
    pass->camera(glm::mat4x4(1.0f), pixels());

    canvas::Line lines;
    canvas::World quads;
    if (drawn == Drawn::Lines) {
        lines.line(glm::vec3(0.0f, 16.5f, 0.5f), glm::vec3(64.0f, 16.5f, 0.5f), red);
        lines.line(glm::vec3(0.0f, 16.5f, 0.5f), glm::vec3(64.0f, 16.5f, 0.5f), green);
        headless->context->lines()->submit(lines, pass.get());
    } else {
        const canvas::World::Corners band{
            glm::vec3(0.0f, 8.0f, 0.5f),
            glm::vec3(64.0f, 8.0f, 0.5f),
            glm::vec3(64.0f, 24.0f, 0.5f),
            glm::vec3(0.0f, 24.0f, 0.5f)
        };
        quads.quad(band, red);
        quads.quad(band, green);
        headless->context->worldQuads()->submit(quads, pass.get());
    }

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

    boost::filesystem::create_directory("data_out");
    if (!capture.write(path)) {
        return "nothing written";
    }
    v3d::image::reader::Png png(headless->logger);
    const boost::shared_ptr<v3d::image::Image> picture = png.read(path);
    if (!picture) {
        return "nothing read";
    }
    const unsigned char* at = picture->data() + (static_cast<std::size_t>(16) * width + 32) * 4;
    if (at[0] == 255 && at[1] == 0 && at[2] == 0) {
        return "red";
    }
    if (at[0] == 0 && at[1] == 255 && at[2] == 0) {
        return "green";
    }
    return "neither";
}

};  // namespace

BOOST_AUTO_TEST_SUITE(depth_write_test)

/**
 * A pass that turns depth writing off lets the second of two coincident lines draw. The depth
 * buffer still holds the clear value when the green line is tested, so it passes.
 **/
BOOST_AUTO_TEST_CASE(a_pass_without_depth_writes_draws_the_second_line) {
    v3d::test::Headless headless(colourFormat, width, height);
    BOOST_TEST(drawCoincident(&headless, Drawn::Lines, false, "data_out/lines_unwritten.png") == "green");
    BOOST_TEST(headless.silent());
}

/**
 * A pass that names nothing draws the lines as their pipeline was built, writing depth, so the
 * first line hides the second.
 **/
BOOST_AUTO_TEST_CASE(a_pass_that_names_nothing_writes_line_depth) {
    v3d::test::Headless headless(colourFormat, width, height);
    BOOST_TEST(drawCoincident(&headless, Drawn::Lines, std::nullopt, "data_out/lines_written.png") == "red");
    BOOST_TEST(headless.silent());
}

/**
 * World quads are built without depth writes, and a pass that asks for them makes the first of
 * two coincident quads hide the second. Without the pass's choice reaching the quads' pipeline,
 * the green quad would draw.
 *
 * Covers the base: efd586c5 made depth writes a pass's choice. Checked by having the recorder
 * set the pipeline's value whatever the pass names, which fails this case.
 **/
BOOST_AUTO_TEST_CASE(a_pass_with_depth_writes_makes_world_quads_write) {
    v3d::test::Headless headless(colourFormat, width, height);
    BOOST_TEST(drawCoincident(&headless, Drawn::WorldQuads, true, "data_out/quads_written.png") == "red");
    BOOST_TEST(headless.silent());
}

/**
 * A pass that names nothing draws world quads as their pipeline was built, without depth
 * writes, so the second quad draws over the first.
 *
 * Covers the base: efd586c5 made depth writes a pass's choice. Checked by building world quads
 * with depth writes on, which fails this case.
 **/
BOOST_AUTO_TEST_CASE(a_pass_that_names_nothing_leaves_world_quads_unwritten) {
    v3d::test::Headless headless(colourFormat, width, height);
    BOOST_TEST(drawCoincident(&headless, Drawn::WorldQuads, std::nullopt, "data_out/quads_unwritten.png") == "green");
    BOOST_TEST(headless.silent());
}

BOOST_AUTO_TEST_SUITE_END()
