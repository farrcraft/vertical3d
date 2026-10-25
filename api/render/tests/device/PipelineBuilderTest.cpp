/**
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 **/

#include <api/render/realtime/vulkan/pipeline/Builder.h>
#include <api/render/realtime/vulkan/pipeline/Cache.h>
#include <api/render/realtime/vulkan/pipeline/Pipeline.h>

#include <algorithm>
#include <cstddef>
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
 * A vertex stage and nothing else, which is all a depth only pipeline needs. It is included as
 * the C initialiser list that v3d_add_shader has glslc write with -mfmt=c.
 **/
const uint32_t vertexShader[] =
#include "shaders/depth.vert.inc"
;  // NOLINT(whitespace/semicolon) - the initialiser it terminates is the include above

/**
 * The same vertex stage with a different body, for a pipeline that differs from the one
 * describe() gives in its stage alone.
 **/
const uint32_t flatShader[] =
#include "shaders/flat.vert.inc"
;  // NOLINT(whitespace/semicolon) - the initialiser it terminates is the include above

/**
 * A builder carrying the one stage and one attribute the shader declares, named so that a
 * failure says which pipeline it was.
 **/
void describe(Builder* builder) {
    builder->name("depth-only")
        .shader(VK_SHADER_STAGE_VERTEX_BIT, vertexShader, sizeof(vertexShader))
        .vertexBinding({.binding = 0, .stride = sizeof(float) * 3})
        .vertexAttribute({.location = 0, .binding = 0, .format = VK_FORMAT_R32G32B32_SFLOAT, .offset = 0});
}

/**
 * Whether the device is lavapipe, which keeps nothing in a pipeline cache. Its cache holds only
 * the header whatever is built against it.
 **/
bool keepsNoCache(const v3d::test::Headless& headless) {
    VkPhysicalDeviceDriverProperties driver{};
    driver.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_DRIVER_PROPERTIES;
    VkPhysicalDeviceProperties2 properties{};
    properties.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_PROPERTIES_2;
    properties.pNext = &driver;
    vkGetPhysicalDeviceProperties2(headless.device->physical(), &properties);
    return driver.driverID == VK_DRIVER_ID_MESA_LLVMPIPE;
}

/**
 * How many bytes the cache would write out, which grows by an entry each time a build
 * compiles something the cache did not already hold.
 **/
std::size_t held(const v3d::test::Headless& headless, const Cache& cache) {
    std::size_t bytes = 0;
    const VkResult result = vkGetPipelineCacheData(headless.device->handle(), cache.handle(), &bytes, nullptr);
    if (result != VK_SUCCESS) {
        throw std::runtime_error("Unable to read the size of the pipeline cache");
    }
    return bytes;
}

/**
 * Build a pipeline against the cache and destroy it, leaving only what the cache kept.
 **/
void buildAndDestroy(const v3d::test::Headless& headless, const Builder& builder, const boost::shared_ptr<Cache>& cache) {
    const Pipeline built = builder.build(cache);
    vkDestroyPipeline(headless.device->handle(), built.pipeline, nullptr);
    vkDestroyPipelineLayout(headless.device->handle(), built.layout, nullptr);
}

BOOST_AUTO_TEST_SUITE(pipeline_builder_test)

/**
 * A pipeline with no colour attachment at all compiles, as a shadow pass requires.
 *
 * The case checks the compile and an empty validation log. Validation would report a
 * colorAttachmentCount that disagreed with pColorAttachmentFormats, or blend attachments on a
 * pipeline that writes no colour.
 **/
BOOST_AUTO_TEST_CASE(a_pipeline_with_no_colour_attachment_compiles) {
    const v3d::test::Headless headless(colourFormat, width, height);

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
    const v3d::test::Headless headless(colourFormat, width, height);

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
    const v3d::test::Headless headless(colourFormat, width, height);

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
    const v3d::test::Headless headless(colourFormat, width, height);

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
    const v3d::test::Headless headless(colourFormat, width, height);

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
    const v3d::test::Headless headless(colourFormat, width, height);

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
    BOOST_REQUIRE_EQUAL(untouched.size(), 2U);
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
    const v3d::test::Headless headless(colourFormat, width, height);

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
    const v3d::test::Headless headless(colourFormat, width, height);

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
    const v3d::test::Headless headless(colourFormat, width, height);

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

/**
 * A pipeline is compiled into the cache it is built against. Asked for a second time, it adds
 * nothing to the cache, and a pipeline that differs from it in one field adds an entry of its
 * own. The case compares no picture. It reads how much the cache holds after each build.
 *
 * The field that differs is the vertex stage. A different stage needs a new compile on any
 * implementation that caches at all. Whether fixed function state such as the cull mode is part
 * of the cache's key is left to the implementation.
 *
 * An implementation may keep nothing in a cache, and lavapipe keeps nothing. On lavapipe the
 * case builds one pipeline and checks the validation layer, but not the cache's size. Every
 * other driver has to grow the cache, so a build that passes no cache still fails there.
 *
 * Covers the base: 3dd24da7 brought the builder's cache argument. Checked by passing a null
 * cache to vkCreateGraphicsPipelines in build(), which leaves the cache empty and fails this
 * case.
 **/
BOOST_AUTO_TEST_CASE(the_same_pipeline_is_compiled_into_the_cache_once) {
    const v3d::test::Headless headless(colourFormat, width, height);

    const boost::shared_ptr<Cache> cache = boost::make_shared<Cache>(headless.device);
    const std::size_t empty = held(headless, *cache);

    Builder first(headless.device);
    describe(&first);
    first.colourFormat(colourFormat);
    buildAndDestroy(headless, first, cache);
    const std::size_t once = held(headless, *cache);
    if (keepsNoCache(headless)) {
        BOOST_TEST_MESSAGE("lavapipe keeps nothing in a pipeline cache, so its size was not checked");
        BOOST_CHECK(headless.silent());
        return;
    }
    BOOST_CHECK_GT(once, empty);

    Builder again(headless.device);
    describe(&again);
    again.colourFormat(colourFormat);
    buildAndDestroy(headless, again, cache);
    BOOST_CHECK_EQUAL(held(headless, *cache), once);

    Builder other(headless.device);
    other.name("flat")
        .shader(VK_SHADER_STAGE_VERTEX_BIT, flatShader, sizeof(flatShader))
        .vertexBinding({.binding = 0, .stride = sizeof(float) * 3})
        .vertexAttribute({.location = 0, .binding = 0, .format = VK_FORMAT_R32G32B32_SFLOAT, .offset = 0})
        .colourFormat(colourFormat);
    buildAndDestroy(headless, other, cache);
    BOOST_CHECK_GT(held(headless, *cache), once);

    BOOST_CHECK(headless.silent());
}

/**
 * Drawing a triangle as its edges needs the fillModeNonSolid feature, which the device turns on
 * where it is offered. A pipeline in that mode then compiles without a validation message.
 *
 * Point mode needs the same feature, and a vertex stage that writes gl_PointSize as well, which
 * this case's shader does not.
 **/
BOOST_AUTO_TEST_CASE(a_line_polygon_mode_compiles_where_the_device_offers_it) {
    const v3d::test::Headless headless(colourFormat, width, height);
    if (!headless.device->nonSolidFill()) {
        BOOST_TEST_MESSAGE("the device does not offer fillModeNonSolid");
        return;
    }

    const boost::shared_ptr<Cache> cache = boost::make_shared<Cache>(headless.device);
    Builder builder(headless.device);
    describe(&builder);
    builder.polygon(VK_POLYGON_MODE_LINE).depth(true, true).depthFormat(depthFormat).colourFormats({});
    BOOST_CHECK_EQUAL(builder.rasterization().polygonMode, VK_POLYGON_MODE_LINE);
    buildAndDestroy(headless, builder, cache);
    BOOST_CHECK(headless.silent());
}

BOOST_AUTO_TEST_SUITE_END()

};  // namespace
