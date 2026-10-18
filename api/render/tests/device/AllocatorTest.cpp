/**
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 **/

#include <api/render/realtime/vulkan/device/Device.h>
#include <api/render/realtime/vulkan/memory/Allocation.h>
#include <api/render/realtime/vulkan/memory/Allocator.h>

#include <boost/test/unit_test.hpp>

#include "Headless.h"

using v3d::render::realtime::vulkan::memory::Allocation;
using v3d::render::realtime::vulkan::memory::Allocator;

namespace {

/**
 * Memory no type offers is refused through the result rather than thrown, so the caller that
 * made the buffer is the one that destroys it. Protected memory is never host visible, so no
 * device offers the pair. Both allocators refuse it the same way, and nothing is left behind.
 **/
BOOST_AUTO_TEST_CASE(memory_no_type_offers_is_refused_without_throwing) {
    for (const Allocator::Kind kind : { Allocator::Kind::Direct, Allocator::Kind::Suballocated }) {
        BOOST_TEST_CONTEXT((kind == Allocator::Kind::Direct ? "direct" : "suballocated")) {
            const v3d::test::Headless headless(VK_FORMAT_R8G8B8A8_UNORM, 4, 4, kind);

            VkBufferCreateInfo info{};
            info.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
            info.size = 64;
            info.usage = VK_BUFFER_USAGE_TRANSFER_SRC_BIT;
            info.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
            VkBuffer buffer = VK_NULL_HANDLE;
            BOOST_REQUIRE(vkCreateBuffer(headless.device->handle(), &info, nullptr, &buffer) == VK_SUCCESS);

            Allocation allocation;
            VkResult result = VK_SUCCESS;
            BOOST_CHECK_NO_THROW(result = headless.device->allocator().bind(buffer,
                VK_MEMORY_PROPERTY_PROTECTED_BIT | VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT, &allocation));
            BOOST_CHECK(result == VK_ERROR_FEATURE_NOT_PRESENT);
            BOOST_CHECK(!allocation.valid());

            vkDestroyBuffer(headless.device->handle(), buffer, nullptr);
            BOOST_CHECK(headless.silent());
        }
    }
}

};  // namespace
