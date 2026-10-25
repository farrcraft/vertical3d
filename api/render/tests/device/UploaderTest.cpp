/**
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 **/

#include <api/render/realtime/DeviceContext.h>
#include <api/render/realtime/vulkan/frame/Ring.h>
#include <api/render/realtime/vulkan/memory/Buffer.h>
#include <api/render/realtime/vulkan/memory/DeviceBuffer.h>
#include <api/render/realtime/vulkan/memory/Uploader.h>

#include <vulkan/vulkan.h>

#include <array>
#include <cstdint>

#include <boost/make_shared.hpp>
#include <boost/shared_ptr.hpp>
#include <boost/smart_ptr/weak_ptr.hpp>
#include <boost/test/unit_test.hpp>

#include "Headless.h"

using v3d::render::realtime::vulkan::frame::Ring;
using v3d::render::realtime::vulkan::memory::Buffer;
using v3d::render::realtime::vulkan::memory::BufferSpec;
using v3d::render::realtime::vulkan::memory::DeviceBuffer;
using v3d::render::realtime::vulkan::memory::Uploader;

namespace {

const VkFormat colourFormat = VK_FORMAT_R8G8B8A8_UNORM;
const uint32_t width = 16;
const uint32_t height = 16;

BOOST_AUTO_TEST_SUITE(uploader_test)

/**
 * An upload returns before its copy has run, and holds what the copy reads until the frame
 * being recorded has finished. That frame is the next to begin, so what it held is freed once
 * framesInFlight frames after it have begun.
 **/
BOOST_AUTO_TEST_CASE(an_upload_holds_its_staging_until_its_frame_has_finished) {
    v3d::test::Headless headless(colourFormat, width, height);
    const boost::shared_ptr<Ring> ring = headless.context->ring();
    headless.submitAndWait(ring->begin());

    boost::shared_ptr<int> staging = boost::make_shared<int>(0);
    const boost::weak_ptr<int> held = staging;
    headless.context->uploader()->submit([](VkCommandBuffer) {}, staging);
    staging.reset();
    BOOST_CHECK(!held.expired());

    for (uint32_t frame = 0; frame < ring->framesInFlight(); frame++) {
        headless.submit(ring->begin());
        BOOST_CHECK(!held.expired());
    }
    headless.submit(ring->begin());
    BOOST_CHECK(held.expired());

    ring->waitIdle();
    BOOST_CHECK(headless.silent());
}

/**
 * An Uploader may go before its uploads have run. What each held is still freed when its frame
 * has finished, and the command buffers it used are freed with their pool after that.
 **/
BOOST_AUTO_TEST_CASE(an_uploader_may_go_before_its_uploads_have_run) {
    v3d::test::Headless headless(colourFormat, width, height);
    const boost::shared_ptr<Ring> ring = headless.context->ring();

    boost::shared_ptr<int> staging = boost::make_shared<int>(0);
    const boost::weak_ptr<int> held = staging;
    {
        Uploader uploader(headless.device, ring);
        uploader.submit([](VkCommandBuffer) {}, staging);
        uploader.submit([](VkCommandBuffer) {});
    }
    staging.reset();
    BOOST_CHECK(!held.expired());

    for (uint32_t frame = 0; frame <= ring->framesInFlight(); frame++) {
        headless.submit(ring->begin());
    }
    BOOST_CHECK(held.expired());

    ring->waitIdle();
    BOOST_CHECK(headless.silent());
}

/**
 * A buffer filled by an upload that did not wait holds what was uploaded when the next frame
 * reads it. The read is a copy into a host buffer, recorded into the frame. Synchronization
 * validation reports a copy that the frame's read is not ordered after.
 *
 * Passes before the change: an upload that waited was ordered before the frame as well. It
 * guards the barrier that makes an upload that does not wait visible to the frame after it.
 **/
BOOST_AUTO_TEST_CASE(an_uploaded_buffer_is_whole_in_the_next_frame) {
    v3d::test::Headless headless(colourFormat, width, height);
    const boost::shared_ptr<Ring> ring = headless.context->ring();

    const std::array<uint32_t, 4> uploaded = { 0x01234567U, 0x89ABCDEFU, 0xFEDCBA98U, 0x76543210U };
    const DeviceBuffer device(headless.device, headless.context->uploader(), VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
        uploaded.data(), sizeof(uploaded));
    Buffer readback(headless.device, BufferSpec{.usage = VK_BUFFER_USAGE_TRANSFER_DST_BIT, .bytes = sizeof(uploaded)});

    VkCommandBuffer commands = ring->begin();
    VkBufferCopy copy{};
    copy.size = sizeof(uploaded);
    vkCmdCopyBuffer(commands, device.handle(), readback.handle(), 1, &copy);

    VkBufferMemoryBarrier2 barrier{};
    barrier.sType = VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER_2;
    barrier.srcStageMask = VK_PIPELINE_STAGE_2_COPY_BIT;
    barrier.srcAccessMask = VK_ACCESS_2_TRANSFER_WRITE_BIT;
    barrier.dstStageMask = VK_PIPELINE_STAGE_2_HOST_BIT;
    barrier.dstAccessMask = VK_ACCESS_2_HOST_READ_BIT;
    barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    barrier.buffer = readback.handle();
    barrier.size = VK_WHOLE_SIZE;
    VkDependencyInfo dependency{};
    dependency.sType = VK_STRUCTURE_TYPE_DEPENDENCY_INFO;
    dependency.bufferMemoryBarrierCount = 1;
    dependency.pBufferMemoryBarriers = &barrier;
    vkCmdPipelineBarrier2(commands, &dependency);
    headless.submitAndWait(commands);

    std::array<uint32_t, 4> read{};
    readback.read(read.data(), sizeof(read));
    BOOST_CHECK_EQUAL_COLLECTIONS(read.begin(), read.end(), uploaded.begin(), uploaded.end());
    BOOST_CHECK(headless.silent());
}

BOOST_AUTO_TEST_SUITE_END()

};  // namespace
