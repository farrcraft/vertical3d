/**
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 **/

#include <api/render/realtime/vulkan/frame/Swapchain.h>

#include <vector>

#include <boost/test/unit_test.hpp>

using v3d::render::realtime::vulkan::frame::Swapchain;

namespace {

/**
 * @return a surface format entry, in the only colour space a chain is presented in
 **/
VkSurfaceFormatKHR offered(VkFormat format, VkColorSpaceKHR space = VK_COLOR_SPACE_SRGB_NONLINEAR_KHR) {
    VkSurfaceFormatKHR entry{};
    entry.format = format;
    entry.colorSpace = space;
    return entry;
}

};  // namespace

BOOST_AUTO_TEST_SUITE(swapchain_format_test)

/**
 * A caller that names no format gets ADR-0009's rule, whatever order the surface lists its
 * formats in. The SRGB entry sits first here because that is the order a driver usually
 * reports and the order a naive pick would take.
 **/
BOOST_AUTO_TEST_CASE(no_preference_takes_unorm) {
    const std::vector<VkSurfaceFormatKHR> formats{
        offered(VK_FORMAT_B8G8R8A8_SRGB),
        offered(VK_FORMAT_B8G8R8A8_UNORM)};

    BOOST_CHECK_EQUAL(Swapchain::chooseFormat(formats).format, VK_FORMAT_B8G8R8A8_UNORM);
}

/**
 * An app that writes linear light asks for a target that encodes, and gets it.
 **/
BOOST_AUTO_TEST_CASE(a_preference_the_surface_offers_wins) {
    const std::vector<VkSurfaceFormatKHR> formats{
        offered(VK_FORMAT_B8G8R8A8_UNORM),
        offered(VK_FORMAT_B8G8R8A8_SRGB)};

    BOOST_CHECK_EQUAL(Swapchain::chooseFormat(formats, VK_FORMAT_B8G8R8A8_SRGB).format, VK_FORMAT_B8G8R8A8_SRGB);
}

/**
 * A chain is built either way. A preference the surface cannot satisfy falls back to the
 * default rather than failing, because a window that presents nothing is worse than one whose
 * colours the app has to encode itself.
 **/
BOOST_AUTO_TEST_CASE(a_preference_the_surface_lacks_falls_back) {
    const std::vector<VkSurfaceFormatKHR> formats{
        offered(VK_FORMAT_R8G8B8A8_UNORM)};

    BOOST_CHECK_EQUAL(Swapchain::chooseFormat(formats, VK_FORMAT_B8G8R8A8_SRGB).format, VK_FORMAT_R8G8B8A8_UNORM);
}

/**
 * The colour space is matched as well as the format. The same format in a wide gamut space is
 * a different target, and taking it would hand the app a chain it did not ask for.
 **/
BOOST_AUTO_TEST_CASE(a_preference_in_another_colour_space_is_not_a_match) {
    const std::vector<VkSurfaceFormatKHR> formats{
        offered(VK_FORMAT_B8G8R8A8_SRGB, VK_COLOR_SPACE_EXTENDED_SRGB_LINEAR_EXT),
        offered(VK_FORMAT_B8G8R8A8_UNORM)};

    BOOST_CHECK_EQUAL(Swapchain::chooseFormat(formats, VK_FORMAT_B8G8R8A8_SRGB).format, VK_FORMAT_B8G8R8A8_UNORM);
}

/**
 * A surface offering neither a preference nor a UNORM format still yields a chain.
 **/
BOOST_AUTO_TEST_CASE(neither_available_takes_the_first_offered) {
    const std::vector<VkSurfaceFormatKHR> formats{
        offered(VK_FORMAT_A2B10G10R10_UNORM_PACK32, VK_COLOR_SPACE_HDR10_ST2084_EXT),
        offered(VK_FORMAT_R16G16B16A16_SFLOAT, VK_COLOR_SPACE_EXTENDED_SRGB_LINEAR_EXT)};

    BOOST_CHECK_EQUAL(Swapchain::chooseFormat(formats).format, VK_FORMAT_A2B10G10R10_UNORM_PACK32);
}

BOOST_AUTO_TEST_SUITE_END()
