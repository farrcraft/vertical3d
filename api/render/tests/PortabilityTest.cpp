/**
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 **/

#include <api/render/realtime/vulkan/device/Device.h>
#include <api/render/realtime/vulkan/device/Instance.h>

#include <vulkan/vulkan.h>

#include <algorithm>
#include <cstring>
#include <string>
#include <vector>

#include <boost/test/unit_test.hpp>

using v3d::render::realtime::vulkan::device::Device;
using v3d::render::realtime::vulkan::device::Instance;

namespace {

/**
 * The extension a windowing system asks every instance for.
 **/
const char* const surface = "VK_KHR_surface";

/**
 * @return whether a list of extension names holds one
 **/
bool holds(const std::vector<const char*>& names, const char* name) {
    return std::any_of(names.begin(), names.end(), [name](const char* held) { return std::strcmp(held, name) == 0; });
}

BOOST_AUTO_TEST_SUITE(portability_test)

/**
 * A loader that offers portability enumeration has it enabled, with the flag that makes it
 * list portability drivers, beside what the windowing system asked for.
 **/
BOOST_AUTO_TEST_CASE(an_instance_enumerates_portability_drivers_where_the_loader_can) {
    const std::vector<std::string> offered{"VK_KHR_surface", VK_KHR_PORTABILITY_ENUMERATION_EXTENSION_NAME,
        VK_EXT_DEBUG_UTILS_EXTENSION_NAME};
    const Instance::Enabling enabling = Instance::enable({surface}, offered, false);

    BOOST_TEST(holds(enabling.extensions, surface));
    BOOST_TEST(holds(enabling.extensions, VK_KHR_PORTABILITY_ENUMERATION_EXTENSION_NAME));
    BOOST_TEST((enabling.flags & VK_INSTANCE_CREATE_ENUMERATE_PORTABILITY_BIT_KHR) != 0U);
    BOOST_TEST(enabling.portability);
    // debug utils is offered, but nothing is validating
    BOOST_TEST(!holds(enabling.extensions, VK_EXT_DEBUG_UTILS_EXTENSION_NAME));
}

/**
 * A loader that does not offer it is not asked for it, and the instance gets no flag. An older
 * loader would refuse an extension it does not know, and this is every conformant desktop
 * driver's case.
 **/
BOOST_AUTO_TEST_CASE(an_instance_asks_for_nothing_a_loader_does_not_offer) {
    const std::vector<std::string> offered{"VK_KHR_surface", VK_EXT_DEBUG_UTILS_EXTENSION_NAME};
    const Instance::Enabling enabling = Instance::enable({surface}, offered, true);

    BOOST_TEST(!holds(enabling.extensions, VK_KHR_PORTABILITY_ENUMERATION_EXTENSION_NAME));
    BOOST_TEST(enabling.flags == 0U);
    BOOST_TEST(!enabling.portability);
    BOOST_TEST(holds(enabling.extensions, surface));
    BOOST_TEST(holds(enabling.extensions, VK_EXT_DEBUG_UTILS_EXTENSION_NAME));
    BOOST_TEST(enabling.extensions.size() == 2U);
}

/**
 * A device that offers the portability subset is created with it, as it must be, whether or
 * not it presents. One that does not is created with what the renderer needs and nothing else.
 **/
BOOST_AUTO_TEST_CASE(a_device_enables_the_portability_subset_it_offers) {
    const std::vector<std::string> portable{VK_KHR_SWAPCHAIN_EXTENSION_NAME, "VK_KHR_portability_subset"};
    const std::vector<std::string> conformant{VK_KHR_SWAPCHAIN_EXTENSION_NAME, "VK_KHR_maintenance5"};

    const std::vector<const char*> presenting = Device::extensions(true, portable);
    BOOST_TEST(holds(presenting, VK_KHR_SWAPCHAIN_EXTENSION_NAME));
    BOOST_TEST(holds(presenting, "VK_KHR_portability_subset"));

    const std::vector<const char*> headless = Device::extensions(false, portable);
    BOOST_TEST(headless.size() == 1U);
    BOOST_TEST(holds(headless, "VK_KHR_portability_subset"));

    const std::vector<const char*> plain = Device::extensions(true, conformant);
    BOOST_TEST(plain.size() == 1U);
    BOOST_TEST(holds(plain, VK_KHR_SWAPCHAIN_EXTENSION_NAME));
}

BOOST_AUTO_TEST_SUITE_END()

};  // namespace
