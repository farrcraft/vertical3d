/**
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 **/

#include <api/render/realtime/DrawItem.h>
#include <api/render/realtime/Frame.h>
#include <api/render/realtime/Pass.h>
#include <api/render/realtime/vulkan/frame/Capture.h>
#include <api/render/realtime/vulkan/frame/Recorder.h>
#include <api/render/realtime/vulkan/frame/RenderTarget.h>
#include <api/render/realtime/vulkan/memory/Barriers.h>
#include <api/render/realtime/vulkan/memory/Buffer.h>
#include <api/render/realtime/vulkan/memory/Image.h>
#include <api/render/realtime/vulkan/pipeline/Builder.h>

#include <cstddef>
#include <optional>
#include <vector>

#include <boost/make_shared.hpp>
#include <boost/test/unit_test.hpp>

#include "Headless.h"

using v3d::render::realtime::DrawItem;
using v3d::render::realtime::Frame;
using v3d::render::realtime::Pass;
using v3d::render::realtime::PipelineHandle;
using v3d::render::realtime::vulkan::frame::Capture;
using v3d::render::realtime::vulkan::frame::Recorder;
using v3d::render::realtime::vulkan::frame::RenderTarget;
using v3d::render::realtime::vulkan::memory::Buffer;
using v3d::render::realtime::vulkan::memory::Image;
using v3d::render::realtime::vulkan::pipeline::Builder;

namespace {

const VkFormat colourFormat = VK_FORMAT_R8G8B8A8_UNORM;
const uint32_t width = 16;
const uint32_t height = 16;

const uint32_t vertexShader[] =
#include "shaders/depth.vert.inc"
;  // NOLINT(whitespace/semicolon) - the initialiser it terminates is the include above

/**
 * Two quads in clip space, each at one depth: the left at a quarter and the right at three
 * quarters. Each is a plane of one depth, so interpolation across it gives that depth at
 * every covered sample. Both values are exact in a 32 bit float, so the case compares equal
 * rather than near.
 **/
const float nearDepth = 0.25f;
const float farDepth = 0.75f;

/**
 * Two triangles spanning x from left to right and y from -0.5 to 0.5, at one depth.
 **/
void quad(std::vector<float>* vertices, float left, float right, float depth) {
    vertices->insert(vertices->end(), {
        left, -0.5f, depth, right, -0.5f, depth, right, 0.5f, depth,
        left, -0.5f, depth, right, 0.5f, depth, left, 0.5f, depth
    });
}

std::vector<float> quads() {
    std::vector<float> vertices;
    quad(&vertices, -0.75f, -0.25f, nearDepth);
    quad(&vertices, 0.25f, 0.75f, farDepth);
    return vertices;
}

/**
 * A depth-only pipeline drawing into the target's format, biased or not.
 **/
PipelineHandle depthPipeline(v3d::test::Headless* headless, VkFormat depthFormat, bool biased) {
    Builder builder(headless->device);
    builder.name(biased ? "biased-depth" : "depth")
        .shader(VK_SHADER_STAGE_VERTEX_BIT, vertexShader, sizeof(vertexShader))
        .vertexBinding(0, sizeof(float) * 3)
        .vertexAttribute(0, 0, VK_FORMAT_R32G32B32_SFLOAT, 0)
        .cull(VK_CULL_MODE_NONE, VK_FRONT_FACE_COUNTER_CLOCKWISE)
        .depth(true, true)
        .depthBias(biased)
        .depthFormat(depthFormat)
        .colourFormats({});
    return headless->context->resources()->add(builder.build(headless->context->pipelineCache()));
}

/**
 * Draw the two quads into a depth-only target and read its depth back, one float a pixel.
 *
 * @param bias the constant bias the pass names, or nothing for an unbiased pipeline
 **/
std::vector<float> drawDepth(v3d::test::Headless* headless, const boost::shared_ptr<RenderTarget>& target,
    const std::optional<float>& bias) {
    const std::vector<float> vertices = quads();
    Buffer buffer(headless->device, VK_BUFFER_USAGE_VERTEX_BUFFER_BIT, vertices.size() * sizeof(float));
    buffer.write(vertices.data(), vertices.size() * sizeof(float));

    Frame frame;
    boost::shared_ptr<Pass> pass = frame.pass("shadow");
    pass->target(target);
    pass->depth(true);
    pass->clearColour(glm::vec4(0.0f));
    if (bias) {
        pass->depthBias(*bias, 0.0f);
    }

    DrawItem item;
    item.pipeline = depthPipeline(headless, target->depthFormat(), bias.has_value());
    item.vertexBuffer = buffer.handle();
    item.vertices = static_cast<uint32_t>(vertices.size() / 3);
    pass->submit(item);

    VkCommandBuffer commands = headless->context->ring()->begin();
    // every pass names a target of its own, so the frame is given no image
    Recorder::record(commands, frame, Recorder::Target(), *headless->context->resources());

    Capture capture(headless->device, headless->logger);
    Capture::Source source;
    source.image = target->depthImage();
    source.extent = target->extent();
    source.format = target->depthFormat();
    source.layout = VK_IMAGE_LAYOUT_DEPTH_READ_ONLY_OPTIMAL;
    source.depth = true;
    capture.record(commands, source);

    headless->submitAndWait(commands);
    return capture.depth();
}

float at(const std::vector<float>& depths, uint32_t x, uint32_t y) {
    return depths[static_cast<std::size_t>(y) * width + x];
}

/**
 * Draw the two quads into a depth-only target from a pass that uses depth, alongside a pass
 * that writes the same target without depth, and read the depth back.
 *
 * @param depthFirst whether the pass with depth comes before the one without
 **/
std::vector<float> drawBeside(v3d::test::Headless* headless, const boost::shared_ptr<RenderTarget>& target,
    bool depthFirst) {
    const std::vector<float> vertices = quads();
    Buffer buffer(headless->device, VK_BUFFER_USAGE_VERTEX_BUFFER_BIT, vertices.size() * sizeof(float));
    buffer.write(vertices.data(), vertices.size() * sizeof(float));

    Frame frame;
    boost::shared_ptr<Pass> without;
    if (!depthFirst) {
        without = frame.pass("without");
    }
    boost::shared_ptr<Pass> with = frame.pass("with");
    if (depthFirst) {
        without = frame.pass("without");
    }
    without->target(target);
    without->depth(false);
    without->keepColour();
    with->target(target);
    with->depth(true);
    with->clearColour(glm::vec4(0.0f));

    DrawItem item;
    item.pipeline = depthPipeline(headless, target->depthFormat(), false);
    item.vertexBuffer = buffer.handle();
    item.vertices = static_cast<uint32_t>(vertices.size() / 3);
    with->submit(item);

    VkCommandBuffer commands = headless->context->ring()->begin();
    Recorder::record(commands, frame, Recorder::Target(), *headless->context->resources());

    Capture capture(headless->device, headless->logger);
    Capture::Source source;
    source.image = target->depthImage();
    source.extent = target->extent();
    source.format = target->depthFormat();
    source.layout = VK_IMAGE_LAYOUT_DEPTH_READ_ONLY_OPTIMAL;
    source.depth = true;
    capture.record(commands, source);

    headless->submitAndWait(commands);
    return capture.depth();
}

};  // namespace

BOOST_AUTO_TEST_SUITE(depth_target_test)

/**
 * A shadow map draws into a target with sampled depth and no colour. The depth a pass writes
 * there is what the specification says it is: the plane's depth wherever a quad covers a
 * pixel, and the clear everywhere else.
 **/
BOOST_AUTO_TEST_CASE(a_depth_only_target_holds_the_depth_drawn) {
    v3d::test::Headless headless(colourFormat, width, height);
    boost::shared_ptr<RenderTarget> target = boost::make_shared<RenderTarget>(headless.device, headless.context->ring(),
        width, height, VK_FORMAT_UNDEFINED, true, true);
    BOOST_REQUIRE(target->image() == VK_NULL_HANDLE);
    if (target->depthFormat() != VK_FORMAT_D32_SFLOAT) {
        BOOST_TEST_MESSAGE("The device gives no sampled D32_SFLOAT, so there is no exact depth to compare");
        return;
    }

    const std::vector<float> depths = drawDepth(&headless, target, std::nullopt);
    BOOST_CHECK(headless.silent());
    BOOST_REQUIRE_EQUAL(depths.size(), static_cast<std::size_t>(width) * height);

    // the centre of each quad, a pixel between them, and a corner
    BOOST_CHECK_EQUAL(at(depths, 4, 8), nearDepth);
    BOOST_CHECK_EQUAL(at(depths, 11, 8), farDepth);
    BOOST_CHECK_EQUAL(at(depths, 8, 8), 1.0f);
    BOOST_CHECK_EQUAL(at(depths, 0, 0), 1.0f);
}

/**
 * A pass's bias reaches a biased pipeline. How far it moves a depth is the implementation's,
 * by way of the format's smallest step, so the case pins only that it moved away from the eye.
 **/
BOOST_AUTO_TEST_CASE(a_pass_bias_moves_the_depth_drawn) {
    v3d::test::Headless headless(colourFormat, width, height);
    boost::shared_ptr<RenderTarget> target = boost::make_shared<RenderTarget>(headless.device, headless.context->ring(),
        width, height, VK_FORMAT_UNDEFINED, true, true);
    if (target->depthFormat() != VK_FORMAT_D32_SFLOAT) {
        BOOST_TEST_MESSAGE("The device gives no sampled D32_SFLOAT, so there is no exact depth to compare");
        return;
    }

    const std::vector<float> depths = drawDepth(&headless, target, 64.0f);
    BOOST_CHECK(headless.silent());
    BOOST_REQUIRE_EQUAL(depths.size(), static_cast<std::size_t>(width) * height);

    BOOST_CHECK_GT(at(depths, 4, 8), nearDepth);
    BOOST_CHECK_LT(at(depths, 4, 8), farDepth);
    BOOST_CHECK_GT(at(depths, 11, 8), farDepth);
    BOOST_CHECK_EQUAL(at(depths, 0, 0), 1.0f);
}

/**
 * A target written by a pass without depth and by one with it has its depth opened and closed
 * once a frame, whichever of the two comes first. The validation layer reports a depth image
 * used in the wrong layout, so the case checks that it is silent and that the depth is right.
 **/
BOOST_AUTO_TEST_CASE(a_pass_without_depth_beside_one_with_it) {
    for (const bool depthFirst : {false, true}) {
        BOOST_TEST_CONTEXT("the pass with depth " << (depthFirst ? "first" : "second")) {
            v3d::test::Headless headless(colourFormat, width, height);
            boost::shared_ptr<RenderTarget> target = boost::make_shared<RenderTarget>(headless.device,
                headless.context->ring(), width, height, VK_FORMAT_UNDEFINED, true, true);
            if (target->depthFormat() != VK_FORMAT_D32_SFLOAT) {
                BOOST_TEST_MESSAGE("The device gives no sampled D32_SFLOAT, so there is no exact depth to compare");
                return;
            }

            const std::vector<float> depths = drawBeside(&headless, target, depthFirst);
            BOOST_CHECK(headless.silent());
            BOOST_REQUIRE_EQUAL(depths.size(), static_cast<std::size_t>(width) * height);
            BOOST_CHECK_EQUAL(at(depths, 4, 8), nearDepth);
            BOOST_CHECK_EQUAL(at(depths, 8, 8), 1.0f);
        }
    }
}

/**
 * A depth image whose format also has stencil is moved by the depth barriers, which name only
 * its depth aspect. The device enables separate depth and stencil layouts, so the layer reports
 * nothing. A device that offers neither combined format as an attachment has nothing to check.
 **/
BOOST_AUTO_TEST_CASE(a_combined_depth_format_moves_by_its_depth_aspect) {
    v3d::test::Headless headless(colourFormat, width, height);

    VkFormat combined = VK_FORMAT_UNDEFINED;
    for (const VkFormat candidate : {VK_FORMAT_D32_SFLOAT_S8_UINT, VK_FORMAT_D24_UNORM_S8_UINT}) {
        VkFormatProperties properties{};
        vkGetPhysicalDeviceFormatProperties(headless.device->physical(), candidate, &properties);
        if ((properties.optimalTilingFeatures & VK_FORMAT_FEATURE_DEPTH_STENCIL_ATTACHMENT_BIT) != 0) {
            combined = candidate;
            break;
        }
    }
    if (combined == VK_FORMAT_UNDEFINED) {
        BOOST_TEST_MESSAGE("the device has no combined depth and stencil attachment format - this case asserts nothing");
        return;
    }

    Image::Spec spec;
    spec.width = width;
    spec.height = height;
    spec.format = combined;
    spec.usage = VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT;
    spec.aspect = VK_IMAGE_ASPECT_DEPTH_BIT;
    const boost::shared_ptr<Image> image = boost::make_shared<Image>(headless.device, spec);

    VkCommandBuffer commands = headless.context->ring()->begin();
    v3d::render::realtime::vulkan::memory::record(commands, {v3d::render::realtime::vulkan::memory::depthForDrawing(image->handle())});
    v3d::render::realtime::vulkan::memory::record(commands, {v3d::render::realtime::vulkan::memory::depthForSampling(image->handle())});
    headless.submitAndWait(commands);

    BOOST_CHECK(headless.silent());
}

BOOST_AUTO_TEST_SUITE_END()
