/**
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 **/

#include <api/render/realtime/DeviceContext.h>
#include <api/render/realtime/Present.h>
#include <api/render/realtime/vulkan/device/Device.h>
#include <api/render/realtime/vulkan/device/Surface.h>
#include <api/render/realtime/vulkan/frame/Swapchain.h>

#include <vulkan/vulkan.h>

#include <cstdint>
#include <vector>

#include <boost/test/unit_test.hpp>

#include "Windowed.h"

namespace {

using v3d::render::realtime::Present;
using v3d::render::realtime::vulkan::frame::Swapchain;

/**
 * The modes the window's surface offers, read from the driver rather than from anything the
 * engine kept, so the expectation does not come from the code under test.
 **/
std::vector<VkPresentModeKHR> offered(const v3d::test::Windowed& windowed) {
    const boost::shared_ptr<v3d::render::realtime::vulkan::device::Device> device = windowed.engine->context()->device();
    uint32_t count = 0;
    vkGetPhysicalDeviceSurfacePresentModesKHR(device->physical(), device->surface()->handle(), &count, nullptr);
    std::vector<VkPresentModeKHR> modes(count);
    vkGetPhysicalDeviceSurfacePresentModesKHR(device->physical(), device->surface()->handle(), &count, modes.data());
    return modes;
}

/**
 * A present mode asked for after the engine started is taken at the next frame, and the engine
 * then reports what the driver granted for it. Each request is checked against what the surface
 * offers, so the case holds on a driver that offers any set of modes.
 **/
BOOST_AUTO_TEST_CASE(engine3d_takes_a_present_mode_at_the_next_frame_test) {
    const v3d::test::Windowed windowed;
    const std::vector<VkPresentModeKHR> modes = offered(windowed);
    BOOST_REQUIRE(!modes.empty());
    BOOST_TEST((windowed.engine->presenting() == Swapchain::granted(Swapchain::choosePresentMode(modes, Present::Uncapped))));

    for (const Present request : {Present::Immediate, Present::AdaptiveVsync, Present::Vsync, Present::Uncapped}) {
        const Present expected = Swapchain::granted(Swapchain::choosePresentMode(modes, request));
        windowed.engine->present(request);
        windowed.engine->renderFrame();
        BOOST_TEST((windowed.engine->presenting() == expected));
        windowed.engine->renderFrame();
    }
    BOOST_TEST(windowed.silent());
}

};  // namespace
