/**
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 **/

#include <api/image/Image.h>
#include <api/image/reader/Png.h>
#include <api/render/realtime/DrawItem.h>
#include <api/render/realtime/Frame.h>
#include <api/render/realtime/Pass.h>
#include <api/render/realtime/vulkan/frame/Capture.h>
#include <api/render/realtime/vulkan/frame/Recorder.h>
#include <api/render/realtime/vulkan/frame/RenderTarget.h>
#include <api/render/realtime/vulkan/memory/Buffer.h>
#include <api/render/realtime/vulkan/pipeline/Builder.h>
#include <api/render/realtime/vulkan/pipeline/DescriptorPool.h>

#include <cstddef>
#include <stdexcept>
#include <vector>

#include <boost/make_shared.hpp>
#include <boost/test/unit_test.hpp>
#include <glm/vec4.hpp>

#include "Headless.h"

using v3d::render::realtime::DrawItem;
using v3d::render::realtime::Frame;
using v3d::render::realtime::Pass;
using v3d::render::realtime::PipelineHandle;
using v3d::render::realtime::vulkan::frame::Capture;
using v3d::render::realtime::vulkan::frame::Recorder;
using v3d::render::realtime::vulkan::frame::RenderTarget;
using v3d::render::realtime::vulkan::memory::Buffer;
using v3d::render::realtime::vulkan::pipeline::Builder;
using v3d::render::realtime::vulkan::pipeline::DescriptorPool;

namespace {

const VkFormat colourFormat = VK_FORMAT_R8G8B8A8_UNORM;
const uint32_t width = 16;
const uint32_t height = 16;

const uint32_t vertexShader[] =
#include "shaders/scene.vert.inc"
;  // NOLINT(whitespace/semicolon) - the initialiser it terminates is the include above

const uint32_t fragmentShader[] =
#include "shaders/scene.frag.inc"
;  // NOLINT(whitespace/semicolon)

/**
 * A pipeline declaring all three sets, which writes whatever colour set 2 holds.
 **/
PipelineHandle scenePipeline(v3d::test::Headless* headless, VkDescriptorSetLayout scene) {
    Builder builder(headless->device);
    builder.name("scene-set")
        .shader(VK_SHADER_STAGE_VERTEX_BIT, vertexShader, sizeof(vertexShader))
        .shader(VK_SHADER_STAGE_FRAGMENT_BIT, fragmentShader, sizeof(fragmentShader))
        .cull(VK_CULL_MODE_NONE, VK_FRONT_FACE_COUNTER_CLOCKWISE)
        .blend(false)
        .set(headless->context->frameUniforms()->layout())
        .set(headless->context->textures()->layout())
        .set(scene)
        .colourFormat(colourFormat);
    return headless->context->resources()->add(builder.build(headless->context->pipelineCache()));
}

/**
 * Record one pass covering the target with the scene pipeline, and read it back.
 **/
void draw(v3d::test::Headless* headless, const boost::shared_ptr<RenderTarget>& target, PipelineHandle pipeline,
    VkDescriptorSet scene, Capture* capture) {
    Frame frame;
    boost::shared_ptr<Pass> pass = frame.pass("scene");
    pass->clearColour(glm::vec4(0.0f, 0.0f, 0.0f, 1.0f));
    pass->scene(scene);

    DrawItem item;
    item.pipeline = pipeline;
    item.vertices = 3;
    pass->submit(item);

    VkCommandBuffer commands = headless->context->ring()->begin();
    Recorder::Target described;
    described.image = target->image();
    described.view = target->view();
    described.extent = target->extent();
    described.finalLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
    Recorder::record(commands, frame, described, *headless->context->resources(),
        headless->context->frameUniforms().get());

    Capture::Source source;
    source.image = target->image();
    source.extent = target->extent();
    source.format = target->format();
    source.layout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
    capture->record(commands, source);

    headless->submitAndWait(commands);
}

};  // namespace

BOOST_AUTO_TEST_SUITE(scene_set_test)

/**
 * What a pass names as its scene is what a pipeline declaring a set 2 reads there.
 *
 * The colour is in channels at the ends of their range and the triangle covers every pixel,
 * so every conformant implementation produces the same texels and the case can check them
 * without a committed picture.
 **/
BOOST_AUTO_TEST_CASE(a_pass_binds_its_scene_at_set_2) {
    v3d::test::Headless headless(colourFormat, width, height);
    boost::shared_ptr<RenderTarget> target = boost::make_shared<RenderTarget>(headless.device, headless.context->ring(), width, height, colourFormat);

    VkDescriptorSetLayoutBinding block{};
    block.binding = 0;
    block.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    block.descriptorCount = 1;
    block.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;
    DescriptorPool pool(headless.device, headless.context->ring(), std::vector<VkDescriptorSetLayoutBinding>{block}, 1, "scene");

    const glm::vec4 colour(0.0f, 1.0f, 1.0f, 1.0f);
    Buffer uniform(headless.device, VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, sizeof(colour));
    uniform.write(&colour, sizeof(colour));

    VkDescriptorSet scene = pool.allocate();
    VkDescriptorBufferInfo buffer{};
    buffer.buffer = uniform.handle();
    buffer.range = sizeof(colour);
    VkWriteDescriptorSet write{};
    write.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    write.dstSet = scene;
    write.dstBinding = 0;
    write.descriptorCount = 1;
    write.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    write.pBufferInfo = &buffer;
    vkUpdateDescriptorSets(headless.device->handle(), 1, &write, 0, nullptr);

    const PipelineHandle pipeline = scenePipeline(&headless, pool.layout());

    Capture capture(headless.device, headless.logger);
    draw(&headless, target, pipeline, scene, &capture);

    BOOST_CHECK(headless.silent());

    BOOST_REQUIRE(capture.write("data_out/scene_set.png"));
    v3d::image::reader::Png png(headless.logger);
    boost::shared_ptr<v3d::image::Image> picture = png.read("data_out/scene_set.png");
    BOOST_REQUIRE(picture);
    const unsigned char* pixels = picture->data();
    for (uint32_t texel = 0; texel < width * height; texel++) {
        const unsigned char* at = pixels + static_cast<std::size_t>(texel) * 4;
        if (at[0] != 0x00 || at[1] != 0xFF || at[2] != 0xFF || at[3] != 0xFF) {
            BOOST_ERROR("texel " << texel << " is not the colour bound at set 2");
            return;
        }
    }
}

/**
 * The same draw in a pass that names no scene is refused before anything is bound, rather
 * than reading whatever set 2 last held.
 **/
BOOST_AUTO_TEST_CASE(a_scene_pipeline_in_a_pass_with_no_scene_throws) {
    v3d::test::Headless headless(colourFormat, width, height);
    boost::shared_ptr<RenderTarget> target = boost::make_shared<RenderTarget>(headless.device, headless.context->ring(), width, height, colourFormat);

    VkDescriptorSetLayoutBinding block{};
    block.binding = 0;
    block.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    block.descriptorCount = 1;
    block.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;
    DescriptorPool pool(headless.device, headless.context->ring(), std::vector<VkDescriptorSetLayoutBinding>{block}, 1, "scene");

    const PipelineHandle pipeline = scenePipeline(&headless, pool.layout());

    Capture capture(headless.device, headless.logger);
    BOOST_CHECK_THROW(draw(&headless, target, pipeline, VK_NULL_HANDLE, &capture), std::runtime_error);
}

BOOST_AUTO_TEST_SUITE_END()
