/**
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 **/

#include <api/image/Compare.h>
#include <api/image/Image.h>
#include <api/image/reader/Png.h>
#include <api/render/realtime/Frame.h>
#include <api/render/realtime/Pass.h>
#include <api/render/realtime/WorldCanvas.h>
#include <api/render/realtime/vulkan/frame/Capture.h>
#include <api/render/realtime/vulkan/frame/Recorder.h>
#include <api/render/realtime/vulkan/frame/RenderTarget.h>

#include <string>

#include <boost/make_shared.hpp>
#include <boost/test/unit_test.hpp>

#include <glm/mat4x4.hpp>
#include <glm/vec3.hpp>
#include <glm/vec4.hpp>

#include "Headless.h"
#include "Reference.h"

using v3d::render::realtime::Frame;
using v3d::render::realtime::Pass;
using v3d::render::realtime::WorldCanvas;
using v3d::render::realtime::vulkan::frame::Capture;
using v3d::render::realtime::vulkan::frame::Recorder;
using v3d::render::realtime::vulkan::frame::RenderTarget;

namespace {

const VkFormat colourFormat = VK_FORMAT_R8G8B8A8_UNORM;
const uint32_t width = 64;
const uint32_t height = 32;

/**
 * A projection that measures the world in pixels of the target: x right over [0, width), y
 * down over [0, height), and z into the screen over [0, 1] the way Vulkan clip space wants
 * it - ADR-0012.
 *
 * Built here rather than through type::camera::Camera because what this case is about is the
 * renderer, and a scale and a translation by exact powers of two put a quad's corners on
 * pixel boundaries with no arithmetic to disagree about - ADR-0054.
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
 * An axis aligned quad at one depth, in the pixel measured space above.
 **/
WorldCanvas::Corners rect(float minX, float minY, float maxX, float maxY, float depth) {
    return WorldCanvas::Corners{
        glm::vec3(minX, minY, depth),
        glm::vec3(maxX, minY, depth),
        glm::vec3(maxX, maxY, depth),
        glm::vec3(minX, maxY, depth)
    };
}

constexpr glm::vec4 NEAR_COLOUR(1.0f, 0.0f, 0.0f, 1.0f);
constexpr glm::vec4 FAR_COLOUR(0.0f, 0.0f, 1.0f, 1.0f);

/**
 * The two overlapping quads, submitted in the order given.
 *
 * The nearer one is at a quarter of the depth range and the farther at three quarters, so
 * which one wins is not a question a rounding could answer differently.
 **/
void overlapping(WorldCanvas* canvas, bool nearFirst) {
    canvas->clear();
    if (nearFirst) {
        canvas->quad(rect(8.0f, 4.0f, 40.0f, 20.0f, 0.25f), NEAR_COLOUR);
        canvas->quad(rect(24.0f, 12.0f, 56.0f, 28.0f, 0.75f), FAR_COLOUR);
    } else {
        canvas->quad(rect(24.0f, 12.0f, 56.0f, 28.0f, 0.75f), FAR_COLOUR);
        canvas->quad(rect(8.0f, 4.0f, 40.0f, 20.0f, 0.25f), NEAR_COLOUR);
    }
}

/**
 * Draw the two quads into a fresh target and compare what came out against the picture
 * committed for that submission order.
 **/
void drawAndCheck(v3d::test::Headless* headless, bool nearFirst, const std::string& name) {
    boost::shared_ptr<RenderTarget> target = boost::make_shared<RenderTarget>(
        headless->device, width, height, colourFormat, true);
    BOOST_REQUIRE(target->depthView() != VK_NULL_HANDLE);

    WorldCanvas canvas;
    overlapping(&canvas, nearFirst);

    Frame frame(headless->context);
    boost::shared_ptr<Pass> pass = frame.pass("world");
    pass->clearColour(glm::vec4(0.0f, 0.0f, 0.0f, 1.0f));
    pass->depth(true);
    pass->camera(glm::mat4x4(1.0f), pixels());

    headless->context->worldQuads()->submit(canvas, pass.get());

    Capture capture(headless->device, headless->logger);

    VkCommandBuffer commands = headless->context->ring()->begin();
    Recorder::Target described;
    described.image = target->image();
    described.view = target->view();
    described.extent = target->extent();
    described.depthImage = target->depthImage();
    described.depthView = target->depthView();
    // not presented, and PRESENT_SRC is not a layout a device with no swapchain extension has
    described.finalLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
    Recorder::record(commands, frame, described, *headless->context->resources(),
        headless->context->frameUniforms().get());

    Capture::Source source;
    source.image = target->image();
    source.extent = target->extent();
    source.format = target->format();
    source.layout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
    capture.record(commands, source);

    headless->submitAndWait(commands);
    headless->context->worldQuads()->endFrame();

    BOOST_CHECK(headless->silent());

    v3d::test::checkReference(headless->logger, &capture, name);
}

/**
 * @return what a case left in data_out, for a comparison between two of its own pictures
 **/
boost::shared_ptr<v3d::image::Image> drawn(const boost::shared_ptr<v3d::log::Logger>& logger, const std::string& name) {
    v3d::image::reader::Png png(logger);
    return png.read("data_out/" + name + ".png");
}

};  // namespace

BOOST_AUTO_TEST_SUITE(world_depth_test)

/**
 * Two overlapping opaque world quads come out in the order they were submitted, whichever
 * way round that is, and the depth between them decides nothing -
 * [ADR-0042](../../../../docs/adr/0042-a-textured-quad-in-world-space.md): the depth tested
 * pipeline tests and does not write, so solid geometry occludes a quad and one quad never
 * occludes another.
 *
 * Each order therefore has a picture of its own and the two differ, which is asserted
 * directly rather than left to a reader comparing two files. A pipeline that wrote depth
 * would make them the same picture, and that is the regression this is here to fail on.
 *
 * The other half of ADR-0042 - the solid geometry that does occlude a quad - is not drawn
 * here, because nothing in this tree writes depth. It needs a pipeline of a consumer's own,
 * the way the depth only pipeline in PipelineBuilderTest is compiled and not drawn with.
 *
 * This is the only case in the tree that reaches renderer::World or a depth attachment on a
 * render target, so it is also what says either works at all.
 **/
BOOST_AUTO_TEST_CASE(world_quads_are_ordered_by_their_caller_and_not_by_depth) {
    v3d::test::Headless headless(colourFormat, width, height);

    drawAndCheck(&headless, true, "world_near_first");
    drawAndCheck(&headless, false, "world_far_first");

    boost::shared_ptr<v3d::image::Image> nearFirst = drawn(headless.logger, "world_near_first");
    boost::shared_ptr<v3d::image::Image> farFirst = drawn(headless.logger, "world_far_first");
    BOOST_REQUIRE(nearFirst != nullptr);
    BOOST_REQUIRE(farFirst != nullptr);
    // the overlap is the far quad's colour in one and the near quad's in the other
    BOOST_CHECK(!v3d::image::compare(*nearFirst, *farFirst, 0).match);
}

BOOST_AUTO_TEST_SUITE_END()
