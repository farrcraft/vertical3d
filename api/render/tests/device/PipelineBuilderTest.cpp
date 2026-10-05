/**
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 **/

#include <api/render/realtime/vulkan/pipeline/Builder.h>
#include <api/render/realtime/vulkan/pipeline/Cache.h>
#include <api/render/realtime/vulkan/pipeline/Pipeline.h>

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
 * A pipeline with no colour attachment at all compiles, as a shadow pass requires.
 *
 * The case checks the compile and an empty validation log. Validation would report a
 * colorAttachmentCount that disagreed with pColorAttachmentFormats, or blend attachments on a
 * pipeline that writes no colour.
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
 * Two colour attachments compile, so the builder accepts any number of attachments, not only
 * none or one. Nothing in this tree draws into two, so only this case covers it.
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
 * A depth only pipeline that offsets what it writes compiles, as a shadow pass requires.
 *
 * The case checks only the compile. Validation does not report VK_DYNAMIC_STATE_DEPTH_BIAS
 * missing from the dynamic list, because the create info's own factors are zero and a zero
 * bias does nothing. This case passes with the dynamic state removed, so
 * a_depth_bias_reaches_vulkan_as_state_and_as_dynamic_state below checks it instead.
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
 * Blend factors chosen by the caller compile. A pipeline given none uses the struct's defaults,
 * which are the straight alpha blend that blend(true) applies.
 *
 * The case uses a destination alpha of ZERO. A pass compositing into a target that is itself
 * composited later then keeps the source's alpha, where the straight alpha default erodes it.
 * The resulting picture is not checked, because a blend is specified to a precision rather
 * than to a value and so has no reference picture.
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
 * A builder given no colour format fails to build. An empty list means a pipeline that writes
 * no colour, while the default means the caller set nothing, and the two must not be confused.
 **/
BOOST_AUTO_TEST_CASE(a_colour_format_nobody_named_is_still_an_error) {
    v3d::test::Headless headless(colourFormat, width, height);

    const boost::shared_ptr<Cache> cache = boost::make_shared<Cache>(headless.device);
    Builder builder(headless.device);
    describe(&builder);

    BOOST_CHECK_THROW(builder.build(cache), std::runtime_error);
}

/**
 * A depth bias reaches Vulkan as both the rasterization flag and the dynamic state, and a
 * builder that sets no bias sets neither.
 *
 * **A compile cannot check this.** A pipeline that enables the bias but leaves
 * VK_DYNAMIC_STATE_DEPTH_BIAS out of the dynamic list compiles and validates without error.
 * The create info's own factors are zero, so vkCmdSetDepthBias then does nothing and a shadow
 * does not shift. A compiled VkPipeline does not expose its dynamic state, so the case reads
 * the state from the builder.
 **/
BOOST_AUTO_TEST_CASE(a_depth_bias_reaches_vulkan_as_state_and_as_dynamic_state) {
    v3d::test::Headless headless(colourFormat, width, height);

    Builder biased(headless.device);
    describe(&biased);
    biased.depth(true, true).depthBias(true).depthFormat(depthFormat).colourFormats({});

    BOOST_CHECK_EQUAL(biased.rasterization().depthBiasEnable, VK_TRUE);
    const std::vector<VkDynamicState> dynamics = biased.dynamics();
    BOOST_CHECK(std::find(dynamics.begin(), dynamics.end(), VK_DYNAMIC_STATE_DEPTH_BIAS) != dynamics.end());

    // the default sets neither, and keeps only the viewport and scissor dynamic states
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
 * Named blend factors arrive in the attachment state as named, and the factors left unnamed
 * are the straight alpha default.
 *
 * Reading the state is the only check of a destination alpha of ZERO. A blend is specified to
 * a precision rather than to a value, so it has no reference picture.
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
    // the three factors left unnamed keep their defaults
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
 * A pass that binds a set once and then draws with several pipelines can share one layout this
 * way. Separate layouts declaring the same sets and push range would also be compatible, but
 * the pass would then hold several identical layouts.
 *
 * **The destroy at the end is the second half of the check.** The builder must not free a
 * layout it was given. The case destroys the layout once, after both pipelines are built and
 * the builders are gone. If a builder had already freed it, the validation layer would report
 * a double free.
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
 * A builder given no layout builds its own, so two such builders give two different layouts.
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
