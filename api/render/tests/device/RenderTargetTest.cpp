/**
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 **/

#include <api/image/Image.h>
#include <api/image/reader/Png.h>
#include <api/render/realtime/Canvas.h>
#include <api/render/realtime/DrawItem.h>
#include <api/render/realtime/Frame.h>
#include <api/render/realtime/Pass.h>
#include <api/render/realtime/vulkan/frame/Capture.h>
#include <api/render/realtime/vulkan/frame/Recorder.h>
#include <api/render/realtime/vulkan/frame/RenderTarget.h>
#include <api/render/realtime/vulkan/memory/Buffer.h>
#include <api/render/realtime/vulkan/pipeline/Builder.h>

#include <cstddef>
#include <stdexcept>
#include <string>
#include <vector>

#include <boost/make_shared.hpp>
#include <boost/test/unit_test.hpp>

#include "Headless.h"

using v3d::render::realtime::Canvas;
using v3d::render::realtime::DrawItem;
using v3d::render::realtime::Frame;
using v3d::render::realtime::Pass;
using v3d::render::realtime::TextureHandle;
using v3d::render::realtime::vulkan::frame::Capture;
using v3d::render::realtime::vulkan::frame::Recorder;
using v3d::render::realtime::vulkan::frame::RenderTarget;
using v3d::render::realtime::vulkan::memory::Buffer;
using v3d::render::realtime::vulkan::pipeline::Builder;

namespace {

const VkFormat colourFormat = VK_FORMAT_R8G8B8A8_UNORM;
const uint32_t width = 16;
const uint32_t height = 16;

const uint32_t vertexShader[] =
#include "shaders/depth.vert.inc"
;  // NOLINT(whitespace/semicolon) - the initialiser it terminates is the include above

/**
 * Capture a target's current image and read the texel at its centre back, as RGBA.
 **/
std::vector<unsigned char> centre(v3d::test::Headless* headless, Capture* capture, const std::string& path) {
    if (!capture->write(path)) {
        return std::vector<unsigned char>();
    }
    v3d::image::reader::Png png(headless->logger);
    boost::shared_ptr<v3d::image::Image> picture = png.read(path);
    if (!picture) {
        return std::vector<unsigned char>();
    }
    const unsigned char* at = picture->data() + (static_cast<std::size_t>(height / 2) * width + width / 2) * 4;
    return std::vector<unsigned char>(at, at + 4);
}

/**
 * One frame: the reader, created first, copies the two-image target's previous() slot into
 * the output, and the writer clears the target's current() slot to a colour. The reader
 * declares what it reads, so the frame records the writer first.
 *
 * @return the texel at the output's centre, which is what the target held a frame ago
 **/
std::vector<unsigned char> drawFrame(v3d::test::Headless* headless, const boost::shared_ptr<RenderTarget>& slots,
    const std::vector<TextureHandle>& handles, const boost::shared_ptr<RenderTarget>& output,
    const glm::vec4& colour, const std::string& path) {
    Canvas canvas;
    canvas.resize(width, height);
    canvas.clear();
    canvas.rect(glm::vec2(0.0f), glm::vec2(width, height), glm::vec2(0.0f), glm::vec2(1.0f), glm::vec4(1.0f),
        handles[slots->previous()]);

    Frame frame;
    boost::shared_ptr<Pass> reader = frame.pass("reader");
    reader->target(output);
    reader->reads(slots);
    reader->clearColour(glm::vec4(0.0f, 0.0f, 1.0f, 1.0f));
    headless->context->quads()->submit(canvas, reader.get());

    boost::shared_ptr<Pass> writer = frame.pass("writer");
    writer->target(slots);
    writer->clearColour(colour);

    VkCommandBuffer commands = headless->context->ring()->begin();
    Recorder::record(commands, frame, Recorder::Target(), *headless->context->resources(),
        headless->context->frameUniforms().get());

    Capture capture(headless->device, headless->logger);
    Capture::Source source;
    source.image = output->image();
    source.extent = output->extent();
    source.format = output->format();
    source.layout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
    capture.record(commands, source);
    headless->submitAndWait(commands);
    headless->context->quads()->endFrame();
    return centre(headless, &capture, path);
}

};  // namespace

BOOST_AUTO_TEST_SUITE(render_target_test)

/**
 * A target of one image per frame in flight draws into a different image each frame, and what
 * the frame before drew is still there to read. On the first frame there is no frame before,
 * and previous() is the cleared image the target was made with rather than an image in an
 * undefined layout. That image is transparent black, so the quad drawn from it blends to
 * nothing and the reader's own blue clear is what shows.
 **/
BOOST_AUTO_TEST_CASE(a_target_per_frame_reads_the_frame_before) {
    v3d::test::Headless headless(colourFormat, width, height);
    const uint32_t frames = headless.context->ring()->framesInFlight();
    BOOST_REQUIRE_GT(frames, 1U);

    boost::shared_ptr<RenderTarget> slots = boost::make_shared<RenderTarget>(headless.device, headless.context->ring(),
        width, height, colourFormat, false, false, frames);
    boost::shared_ptr<RenderTarget> output = boost::make_shared<RenderTarget>(headless.device, headless.context->ring(),
        width, height, colourFormat);
    BOOST_REQUIRE_EQUAL(slots->images(), frames);

    std::vector<TextureHandle> handles;
    handles.reserve(frames);
    for (uint32_t slot = 0; slot < frames; slot++) {
        handles.push_back(headless.context->quads()->texture(*slots, slot));
    }

    VkImage first = slots->image();
    const std::vector<unsigned char> unwritten =
        drawFrame(&headless, slots, handles, output, glm::vec4(1.0f, 0.0f, 0.0f, 1.0f), "data_out/target_first.png");
    VkImage second = slots->image();
    const std::vector<unsigned char> red =
        drawFrame(&headless, slots, handles, output, glm::vec4(0.0f, 1.0f, 0.0f, 1.0f), "data_out/target_second.png");
    const std::vector<unsigned char> green =
        drawFrame(&headless, slots, handles, output, glm::vec4(0.0f, 0.0f, 0.0f, 1.0f), "data_out/target_third.png");

    BOOST_CHECK(headless.silent());
    BOOST_CHECK(first != second);
    BOOST_CHECK(unwritten == (std::vector<unsigned char>{0, 0, 255, 255}));
    BOOST_CHECK(red == (std::vector<unsigned char>{255, 0, 0, 255}));
    BOOST_CHECK(green == (std::vector<unsigned char>{0, 255, 0, 255}));
}

/**
 * Only one image or one per frame in flight: any other count would make previous() a frame
 * that is not the one before.
 **/
BOOST_AUTO_TEST_CASE(a_target_holds_one_image_or_one_per_frame) {
    v3d::test::Headless headless(colourFormat, width, height);
    const uint32_t frames = headless.context->ring()->framesInFlight();
    BOOST_CHECK_THROW(RenderTarget(headless.device, headless.context->ring(), width, height, colourFormat, false, false,
        frames + 1), std::runtime_error);
}

/**
 * A pipeline built for one colour format, drawn into a target of another, is reported by the
 * recorder with the pass that did it - ADR-0068.
 **/
BOOST_AUTO_TEST_CASE(a_pipeline_for_another_format_throws) {
    v3d::test::Headless headless(colourFormat, width, height);
    boost::shared_ptr<RenderTarget> target = boost::make_shared<RenderTarget>(headless.device, headless.context->ring(),
        width, height, VK_FORMAT_B8G8R8A8_UNORM);

    Builder builder(headless.device);
    builder.name("rgba")
        .shader(VK_SHADER_STAGE_VERTEX_BIT, vertexShader, sizeof(vertexShader))
        .vertexBinding(0, sizeof(float) * 3)
        .vertexAttribute(0, 0, VK_FORMAT_R32G32B32_SFLOAT, 0)
        .cull(VK_CULL_MODE_NONE, VK_FRONT_FACE_COUNTER_CLOCKWISE)
        .colourFormat(colourFormat);
    const std::vector<float> triangle{-1.0f, -1.0f, 0.5f, 3.0f, -1.0f, 0.5f, -1.0f, 3.0f, 0.5f};
    Buffer buffer(headless.device, VK_BUFFER_USAGE_VERTEX_BUFFER_BIT, triangle.size() * sizeof(float));
    buffer.write(triangle.data(), triangle.size() * sizeof(float));

    DrawItem item;
    item.pipeline = headless.context->resources()->add(builder.build(headless.context->pipelineCache()));
    item.vertexBuffer = buffer.handle();
    item.vertices = 3;

    Frame frame;
    boost::shared_ptr<Pass> pass = frame.pass("mismatched");
    pass->target(target);
    pass->submit(item);

    VkCommandBuffer commands = headless.context->ring()->begin();
    BOOST_CHECK_THROW(Recorder::record(commands, frame, Recorder::Target(), *headless.context->resources()),
        std::runtime_error);
}

BOOST_AUTO_TEST_SUITE_END()
