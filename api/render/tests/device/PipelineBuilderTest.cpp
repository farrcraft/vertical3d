/**
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 **/

#include <api/render/realtime/vulkan/pipeline/Builder.h>
#include <api/render/realtime/vulkan/pipeline/Cache.h>
#include <api/render/realtime/vulkan/pipeline/Resources.h>

#include <algorithm>
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
 * A depth only pipeline that offsets what it writes compiles - which is what a shadow pass
 * actually is, and what the attachment list alone was not enough to express.
 *
 * The compile is the whole assertion, and it is a narrow one: validation has nothing to say
 * here about VK_DYNAMIC_STATE_DEPTH_BIAS being left out of the dynamic list, because the
 * create info's own factors are zero and a zero bias is a no-op. Measured rather than
 * assumed - this case passes unchanged with the dynamic state removed, which is what
 * a_depth_bias_reaches_vulkan_as_state_and_as_dynamic_state is below for.
 **/
BOOST_AUTO_TEST_CASE(a_depth_only_pipeline_with_a_bias_compiles) {
    v3d::test::Headless headless(colourFormat, width, height);

    const boost::shared_ptr<Cache> cache = boost::make_shared<Cache>(headless.device);
    Builder builder(headless.device);
    describe(&builder);
    builder.depth(true, true).depthBias(true).depthFormat(depthFormat).colourFormats({});

    const Pipeline built = builder.build(cache);
    BOOST_CHECK(built.pipeline != VK_NULL_HANDLE);
    BOOST_CHECK(headless.silent());

    vkDestroyPipeline(headless.device->handle(), built.pipeline, nullptr);
    vkDestroyPipelineLayout(headless.device->handle(), built.layout, nullptr);
}

/**
 * Blend factors of the caller's own compile, and a pipeline given none still blends the way
 * it always has - the struct's defaults are what blend(true) has always meant.
 *
 * A destination alpha of ZERO is the case this exists for: a pass compositing into something
 * that is itself composited later keeps the source's alpha, where the straight alpha default
 * erodes it. What it comes out looking like is not asserted here and cannot be - a blend is
 * specified to a precision rather than to a value, so ADR-0054 gives it no reference.
 **/
BOOST_AUTO_TEST_CASE(a_pipeline_with_named_blend_factors_compiles) {
    v3d::test::Headless headless(colourFormat, width, height);

    const boost::shared_ptr<Cache> cache = boost::make_shared<Cache>(headless.device);
    Builder builder(headless.device);
    describe(&builder);

    Builder::Blend compositing;
    compositing.destinationAlpha = VK_BLEND_FACTOR_ZERO;
    builder.colourFormat(colourFormat).blend(compositing);

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

/**
 * A depth bias reaches Vulkan as both halves of what it takes, and neither half arrives
 * unasked.
 *
 * **This is the assertion a consumer needs and a compile cannot make.** A pipeline that
 * enables the bias but leaves VK_DYNAMIC_STATE_DEPTH_BIAS out of the dynamic list compiles
 * silently and validates silently - the create info's own factors are zero, so
 * vkCmdSetDepthBias then does nothing and a shadow does not shift. Nothing about a compiled
 * VkPipeline says which of the two it got, so the state is read from the builder that will
 * hand it over.
 **/
BOOST_AUTO_TEST_CASE(a_depth_bias_reaches_vulkan_as_state_and_as_dynamic_state) {
    v3d::test::Headless headless(colourFormat, width, height);

    Builder biased(headless.device);
    describe(&biased);
    biased.depth(true, true).depthBias(true).depthFormat(depthFormat).colourFormats({});

    BOOST_CHECK_EQUAL(biased.rasterization().depthBiasEnable, VK_TRUE);
    const std::vector<VkDynamicState> dynamics = biased.dynamics();
    BOOST_CHECK(std::find(dynamics.begin(), dynamics.end(), VK_DYNAMIC_STATE_DEPTH_BIAS) != dynamics.end());

    // and the default disturbs neither, so the rest of the tree is provably where it was
    Builder plain(headless.device);
    describe(&plain);
    plain.depth(true, true).depthFormat(depthFormat).colourFormats({});

    BOOST_CHECK_EQUAL(plain.rasterization().depthBiasEnable, VK_FALSE);
    const std::vector<VkDynamicState> untouched = plain.dynamics();
    BOOST_CHECK(std::find(untouched.begin(), untouched.end(), VK_DYNAMIC_STATE_DEPTH_BIAS) == untouched.end());
    BOOST_REQUIRE_EQUAL(untouched.size(), 2u);
    BOOST_CHECK_EQUAL(untouched[0], VK_DYNAMIC_STATE_VIEWPORT);
    BOOST_CHECK_EQUAL(untouched[1], VK_DYNAMIC_STATE_SCISSOR);
}

/**
 * Named blend factors arrive in the attachment state as named, and the ones nobody names are
 * the straight alpha the tree has always applied.
 *
 * A destination alpha of ZERO is the case this exists for, and it is the field that cannot be
 * checked any other way: ADR-0054 gives a blend no reference picture, because a blend is
 * specified to a precision rather than to a value.
 **/
BOOST_AUTO_TEST_CASE(blend_factors_reach_the_attachment_as_named) {
    v3d::test::Headless headless(colourFormat, width, height);

    Builder named(headless.device);
    describe(&named);
    Builder::Blend compositing;
    compositing.destinationAlpha = VK_BLEND_FACTOR_ZERO;
    named.blend(true).blend(compositing).colourFormat(colourFormat);

    const VkPipelineColorBlendAttachmentState state = named.colourBlend();
    BOOST_CHECK_EQUAL(state.blendEnable, VK_TRUE);
    BOOST_CHECK_EQUAL(state.dstAlphaBlendFactor, VK_BLEND_FACTOR_ZERO);
    // the three nobody moved are still what they were
    BOOST_CHECK_EQUAL(state.srcColorBlendFactor, VK_BLEND_FACTOR_SRC_ALPHA);
    BOOST_CHECK_EQUAL(state.dstColorBlendFactor, VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA);
    BOOST_CHECK_EQUAL(state.srcAlphaBlendFactor, VK_BLEND_FACTOR_ONE);

    Builder plain(headless.device);
    describe(&plain);
    plain.blend(true).colourFormat(colourFormat);

    const VkPipelineColorBlendAttachmentState straight = plain.colourBlend();
    BOOST_CHECK_EQUAL(straight.dstAlphaBlendFactor, VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA);
    BOOST_CHECK_EQUAL(straight.srcColorBlendFactor, VK_BLEND_FACTOR_SRC_ALPHA);
    BOOST_CHECK_EQUAL(straight.dstColorBlendFactor, VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA);
    BOOST_CHECK_EQUAL(straight.srcAlphaBlendFactor, VK_BLEND_FACTOR_ONE);
}

/**
 * One layout the caller owns is compiled into every pipeline given it, and comes back in each
 * of them so that registering the result names the layout its draws bind through.
 *
 * **This is the arrangement a pass wants when it binds a set once and then draws with several
 * pipelines under it**, which is where a layout built per pipeline stops being an obvious
 * equivalent: layouts declaring the same sets and the same push range are compatible, so the
 * binding would survive either way, but a pass that means to share one would hold several that
 * differ in nothing.
 *
 * **The second half of the assertion is the destroy at the end.** The builder must not free a
 * layout it was handed - it is destroyed once here, after both pipelines were built from it and
 * after the builders are gone, and a builder that had freed it would make that a double free
 * the layer reports rather than a leak nobody sees.
 **/
BOOST_AUTO_TEST_CASE(one_layout_the_caller_owns_serves_every_pipeline_given_it) {
    v3d::test::Headless headless(colourFormat, width, height);

    const boost::shared_ptr<Cache> cache = boost::make_shared<Cache>(headless.device);

    VkPipelineLayoutCreateInfo info{};
    info.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
    VkPipelineLayout mine = VK_NULL_HANDLE;
    BOOST_REQUIRE_EQUAL(vkCreatePipelineLayout(headless.device->handle(), &info, nullptr, &mine), VK_SUCCESS);

    Pipeline first;
    Pipeline second;
    {
        Builder lines(headless.device);
        describe(&lines);
        lines.layout(mine).topology(VK_PRIMITIVE_TOPOLOGY_LINE_LIST).colourFormat(colourFormat);
        first = lines.build(cache);

        Builder triangles(headless.device);
        describe(&triangles);
        triangles.layout(mine).colourFormat(colourFormat);
        second = triangles.build(cache);
    }

    BOOST_CHECK(first.pipeline != VK_NULL_HANDLE);
    BOOST_CHECK(second.pipeline != VK_NULL_HANDLE);
    BOOST_CHECK_EQUAL(first.layout, mine);
    BOOST_CHECK_EQUAL(second.layout, mine);
    BOOST_CHECK(first.pipeline != second.pipeline);
    BOOST_CHECK(headless.silent());

    vkDestroyPipeline(headless.device->handle(), second.pipeline, nullptr);
    vkDestroyPipeline(headless.device->handle(), first.pipeline, nullptr);
    vkDestroyPipelineLayout(headless.device->handle(), mine, nullptr);
    BOOST_CHECK(headless.silent());
}

/**
 * A builder nobody hands a layout still builds its own, and two of them are two layouts - so
 * the default this adds a door beside is exactly where it was.
 **/
BOOST_AUTO_TEST_CASE(a_builder_given_no_layout_still_builds_its_own) {
    v3d::test::Headless headless(colourFormat, width, height);

    const boost::shared_ptr<Cache> cache = boost::make_shared<Cache>(headless.device);

    Builder first(headless.device);
    describe(&first);
    first.colourFormat(colourFormat);
    const Pipeline one = first.build(cache);

    Builder second(headless.device);
    describe(&second);
    second.colourFormat(colourFormat);
    const Pipeline two = second.build(cache);

    BOOST_CHECK(one.layout != VK_NULL_HANDLE);
    BOOST_CHECK(two.layout != VK_NULL_HANDLE);
    BOOST_CHECK(one.layout != two.layout);
    BOOST_CHECK(headless.silent());

    vkDestroyPipeline(headless.device->handle(), one.pipeline, nullptr);
    vkDestroyPipeline(headless.device->handle(), two.pipeline, nullptr);
    vkDestroyPipelineLayout(headless.device->handle(), one.layout, nullptr);
    vkDestroyPipelineLayout(headless.device->handle(), two.layout, nullptr);
}

BOOST_AUTO_TEST_SUITE_END()
