/**
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 **/

#include <api/render/realtime/vulkan/pipeline/DescriptorPool.h>

#include <algorithm>
#include <vector>

#include <boost/test/unit_test.hpp>

#include "Headless.h"

using v3d::render::realtime::vulkan::pipeline::DescriptorPool;

namespace {

const VkFormat colourFormat = VK_FORMAT_R8G8B8A8_UNORM;
const uint32_t width = 16;
const uint32_t height = 16;

/**
 * A layout of one uniform buffer, the shape of set 0.
 **/
std::vector<VkDescriptorSetLayoutBinding> uniformBlock() {
    VkDescriptorSetLayoutBinding block{};
    block.binding = 0;
    block.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    block.descriptorCount = 1;
    block.stageFlags = VK_SHADER_STAGE_VERTEX_BIT;
    return std::vector<VkDescriptorSetLayoutBinding>{block};
}

};  // namespace

BOOST_AUTO_TEST_SUITE(descriptor_pool_test)

/**
 * A pool adds a vulkan pool each time the last one is full, so a hundred sets out of pools
 * of 32 take four.
 **/
BOOST_AUTO_TEST_CASE(a_pool_grows_when_the_last_one_is_full) {
    v3d::test::Headless headless(colourFormat, width, height);
    DescriptorPool pool(headless.device, headless.context->ring(), uniformBlock(), 32, "test");

    for (int count = 0; count < 100; count++) {
        BOOST_REQUIRE(pool.allocate() != VK_NULL_HANDLE);
    }

    BOOST_CHECK_EQUAL(pool.pools(), 4U);
    BOOST_CHECK(headless.silent());
}

/**
 * A released set is given out again once the frames in flight have moved past it, rather
 * than another being allocated - the pools cannot free one, so a set not reused is a set
 * leaked until the pool goes.
 **/
BOOST_AUTO_TEST_CASE(a_released_set_is_given_out_again) {
    v3d::test::Headless headless(colourFormat, width, height);
    DescriptorPool pool(headless.device, headless.context->ring(), uniformBlock(), 32, "test");

    std::vector<VkDescriptorSet> sets;
    sets.reserve(100);
    for (int count = 0; count < 100; count++) {
        sets.push_back(pool.allocate());
    }
    std::vector<VkDescriptorSet> released(sets.begin(), sets.begin() + 50);
    for (VkDescriptorSet set : released) {
        pool.release(set);
    }

    // a set comes back only once every frame that began before the release has finished
    for (uint32_t frame = 0; frame <= headless.context->ring()->framesInFlight(); frame++) {
        headless.submit(headless.context->ring()->begin());
    }
    headless.context->ring()->waitIdle();

    for (int count = 0; count < 50; count++) {
        VkDescriptorSet set = pool.allocate();
        BOOST_CHECK(std::find(released.begin(), released.end(), set) != released.end());
    }

    BOOST_CHECK_EQUAL(pool.pools(), 4U);
    BOOST_CHECK(headless.silent());
}

/**
 * A released set is not given out while a frame that began before the release may still be
 * binding it.
 **/
BOOST_AUTO_TEST_CASE(a_released_set_waits_for_the_frames_in_flight) {
    v3d::test::Headless headless(colourFormat, width, height);
    DescriptorPool pool(headless.device, headless.context->ring(), uniformBlock(), 32, "test");

    headless.submit(headless.context->ring()->begin());
    VkDescriptorSet first = pool.allocate();
    pool.release(first);

    BOOST_CHECK(pool.allocate() != first);
    headless.context->ring()->waitIdle();
}

BOOST_AUTO_TEST_SUITE_END()
