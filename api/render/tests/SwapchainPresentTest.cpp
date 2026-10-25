/**
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 **/

#include <api/render/realtime/Present.h>
#include <api/render/realtime/vulkan/frame/Swapchain.h>

#include <vector>

#include <boost/test/unit_test.hpp>

using v3d::render::realtime::Present;
using v3d::render::realtime::vulkan::frame::Swapchain;

namespace {

/**
 * The only mode every implementation has to offer.
 **/
const std::vector<VkPresentModeKHR> fifoOnly{VK_PRESENT_MODE_FIFO_KHR};

/**
 * What a desktop driver commonly offers, listed in an order that puts fifo first.
 **/
const std::vector<VkPresentModeKHR> fifoAndMailbox{VK_PRESENT_MODE_FIFO_KHR, VK_PRESENT_MODE_MAILBOX_KHR};

/**
 * Every mode a request can name.
 **/
const std::vector<VkPresentModeKHR> allFour{VK_PRESENT_MODE_FIFO_RELAXED_KHR, VK_PRESENT_MODE_FIFO_KHR, VK_PRESENT_MODE_IMMEDIATE_KHR, VK_PRESENT_MODE_MAILBOX_KHR};

/**
 * Every request.
 **/
const Present requests[] = {Present::Uncapped, Present::Vsync, Present::AdaptiveVsync, Present::Immediate};

BOOST_AUTO_TEST_SUITE(swapchain_present_test)

/**
 * A surface that offers only fifo grants it whatever is asked for, so every request has
 * somewhere to land.
 **/
BOOST_AUTO_TEST_CASE(fifo_alone_answers_every_request) {
    for (const Present request : requests) {
        BOOST_CHECK_EQUAL(Swapchain::choosePresentMode(fifoOnly, request), VK_PRESENT_MODE_FIFO_KHR);
    }
}

/**
 * With fifo and mailbox, a request for vsync takes fifo though mailbox is offered, and a
 * request for a mode that is not offered falls back as Present says: adaptive vsync to vsync,
 * and immediate to uncapped.
 **/
BOOST_AUTO_TEST_CASE(a_request_falls_back_to_what_is_offered) {
    BOOST_CHECK_EQUAL(Swapchain::choosePresentMode(fifoAndMailbox, Present::Uncapped), VK_PRESENT_MODE_MAILBOX_KHR);
    BOOST_CHECK_EQUAL(Swapchain::choosePresentMode(fifoAndMailbox, Present::Vsync), VK_PRESENT_MODE_FIFO_KHR);
    BOOST_CHECK_EQUAL(Swapchain::choosePresentMode(fifoAndMailbox, Present::AdaptiveVsync), VK_PRESENT_MODE_FIFO_KHR);
    BOOST_CHECK_EQUAL(Swapchain::choosePresentMode(fifoAndMailbox, Present::Immediate), VK_PRESENT_MODE_MAILBOX_KHR);
}

/**
 * A surface that offers every mode grants each request its own, and the mode granted reads
 * back as the request it answers.
 **/
BOOST_AUTO_TEST_CASE(every_request_is_granted_where_every_mode_is_offered) {
    BOOST_CHECK_EQUAL(Swapchain::choosePresentMode(allFour, Present::Uncapped), VK_PRESENT_MODE_MAILBOX_KHR);
    BOOST_CHECK_EQUAL(Swapchain::choosePresentMode(allFour, Present::Vsync), VK_PRESENT_MODE_FIFO_KHR);
    BOOST_CHECK_EQUAL(Swapchain::choosePresentMode(allFour, Present::AdaptiveVsync), VK_PRESENT_MODE_FIFO_RELAXED_KHR);
    BOOST_CHECK_EQUAL(Swapchain::choosePresentMode(allFour, Present::Immediate), VK_PRESENT_MODE_IMMEDIATE_KHR);
    for (const Present request : requests) {
        BOOST_CHECK(Swapchain::granted(Swapchain::choosePresentMode(allFour, request)) == request);
    }
}

/**
 * What is granted on a surface that does not offer the request reads back as the mode in use,
 * not as the request.
 **/
BOOST_AUTO_TEST_CASE(a_fallback_reads_back_as_what_was_granted) {
    BOOST_CHECK(Swapchain::granted(Swapchain::choosePresentMode(fifoOnly, Present::Uncapped)) == Present::Vsync);
    BOOST_CHECK(Swapchain::granted(Swapchain::choosePresentMode(fifoAndMailbox, Present::Immediate)) == Present::Uncapped);
}

BOOST_AUTO_TEST_SUITE_END()

};  // namespace
