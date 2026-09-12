/**
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 **/

#include <api/render/realtime/vulkan/pipeline/Builder.h>
#include <api/render/realtime/vulkan/pipeline/Cache.h>
#include <api/render/realtime/vulkan/pipeline/Resources.h>

#include <cstdint>
#include <stdexcept>
#include <vector>

#include <boost/make_shared.hpp>
#include <boost/test/unit_test.hpp>

#include "Headless.h"

using v3d::render::realtime::vulkan::pipeline::Builder;
using v3d::render::realtime::vulkan::pipeline::Cache;
using v3d::render::realtime::vulkan::pipeline::Pipeline;

namespace {

const VkFormat colourFormat = VK_FORMAT_R8G8B8A8_UNORM;
const VkFormat depthFormat = VK_FORMAT_D32_SFLOAT;
const uint32_t width = 64;
const uint32_t height = 32;

/**
 * A vertex stage and nothing else, which is all a depth only pipeline needs. Included as the
 * C initialiser list glslc's -mfmt=c writes - see v3d_add_shader.
 **/
const uint32_t vertexShader[] =
#include "shaders/depth.vert.inc"
;  // NOLINT(whitespace/semicolon) - the initialiser it terminates is the include above

/**
 * A builder carrying the one stage and one attribute the shader declares, named so that a
 * failure says which pipeline it was.
 **/
void describe(Builder* builder) {
    builder->name("depth-only")
        .shader(VK_SHADER_STAGE_VERTEX_BIT, vertexShader, sizeof(vertexShader))
        .vertexBinding(0, sizeof(float) * 3)
        .vertexAttribute(0, 0, VK_FORMAT_R32G32B32_SFLOAT, 0);
}

};  // namespace

BOOST_AUTO_TEST_SUITE(pipeline_builder_test)

/**
 * A pipeline with no colour attachment at all compiles, which is what a shadow pass is and
 * what the builder could not express while the attachment count was the literal 1.
 *
 * The assertion is the compile plus the layer's silence: a colorAttachmentCount that
 * disagreed with pColorAttachmentFormats, or a blend state with attachments a pipeline
 * writing no colour has no use for, is what validation would report here.
 **/
BOOST_AUTO_TEST_CASE(a_pipeline_with_no_colour_attachment_compiles) {
    v3d::test::Headless headless(colourFormat, width, height);

    const boost::shared_ptr<Cache> cache = boost::make_shared<Cache>(headless.device);
    Builder builder(headless.device);
    describe(&builder);
    builder.depth(true, true).depthFormat(depthFormat).colourFormats({});

    const Pipeline built = builder.build(cache);
    BOOST_CHECK(built.pipeline != VK_NULL_HANDLE);
    BOOST_CHECK(built.layout != VK_NULL_HANDLE);
    BOOST_CHECK(headless.silent());

    vkDestroyPipeline(headless.device->handle(), built.pipeline, nullptr);
    vkDestroyPipelineLayout(headless.device->handle(), built.layout, nullptr);
}

/**
 * Two colour attachments compile, so the list is a list rather than a switch between none and
 * one. Nothing in this tree draws into two, which is why the case is here rather than proven
 * by a renderer.
 **/
BOOST_AUTO_TEST_CASE(a_pipeline_with_two_colour_attachments_compiles) {
    v3d::test::Headless headless(colourFormat, width, height);

    const boost::shared_ptr<Cache> cache = boost::make_shared<Cache>(headless.device);
    Builder builder(headless.device);
    describe(&builder);
    builder.colourFormats({colourFormat, colourFormat});

    const Pipeline built = builder.build(cache);
    BOOST_CHECK(built.pipeline != VK_NULL_HANDLE);
    BOOST_CHECK(headless.silent());

    vkDestroyPipeline(headless.device->handle(), built.pipeline, nullptr);
    vkDestroyPipelineLayout(headless.device->handle(), built.layout, nullptr);
}

/**
 * A builder nobody told about colour still fails, which is the guard colourFormat() has
 * always had. An empty list is a pipeline that writes no colour; the default is one nobody
 * filled in, and the two must not read the same.
 **/
BOOST_AUTO_TEST_CASE(a_colour_format_nobody_named_is_still_an_error) {
    v3d::test::Headless headless(colourFormat, width, height);

    const boost::shared_ptr<Cache> cache = boost::make_shared<Cache>(headless.device);
    Builder builder(headless.device);
    describe(&builder);

    BOOST_CHECK_THROW(builder.build(cache), std::runtime_error);
}

BOOST_AUTO_TEST_SUITE_END()
