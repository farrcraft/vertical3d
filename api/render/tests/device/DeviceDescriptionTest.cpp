/**
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 **/

#include <api/render/realtime/vulkan/device/Device.h>

#include <vulkan/vulkan.h>

#include <string>

#include <boost/test/unit_test.hpp>

#include "Headless.h"

BOOST_AUTO_TEST_SUITE(device_description_test)

/**
 * The description reads the same as the properties the driver reports for the device, with the
 * version split into its parts. Checking only that the name is not empty would pass a version
 * that was never filled in.
 **/
BOOST_AUTO_TEST_CASE(a_device_describes_itself_as_its_driver_does) {
    v3d::test::Headless headless(VK_FORMAT_R8G8B8A8_UNORM, 4, 4);
    VkPhysicalDeviceProperties properties{};
    vkGetPhysicalDeviceProperties(headless.device->physical(), &properties);

    const v3d::render::realtime::vulkan::device::Device::Description& description = headless.device->description();
    BOOST_TEST(description.name == std::string(properties.deviceName));
    BOOST_TEST(description.apiMajor == VK_API_VERSION_MAJOR(properties.apiVersion));
    BOOST_TEST(description.apiMinor == VK_API_VERSION_MINOR(properties.apiVersion));
    BOOST_TEST(description.apiPatch == VK_API_VERSION_PATCH(properties.apiVersion));
    BOOST_TEST(description.vendor == properties.vendorID);
    BOOST_TEST(description.driver == properties.driverVersion);
    // the renderer requires 1.3, so a device it chose is at least that
    BOOST_TEST(description.apiMajor == 1u);
    BOOST_TEST(description.apiMinor >= 3u);
}

BOOST_AUTO_TEST_SUITE_END()
