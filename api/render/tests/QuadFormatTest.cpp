/**
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 **/

#include <api/render/realtime/vulkan/renderer/Quad.h>

#include <boost/test/unit_test.hpp>

using v3d::render::realtime::vulkan::renderer::Quad;

BOOST_AUTO_TEST_SUITE(quad_format_test)

/**
 * Every 8-bit _SRGB colour format decodes a quad first. B8G8R8A8_SRGB is the usual swapchain
 * format, so a quad drawn straight to the window takes this path.
 **/
BOOST_AUTO_TEST_CASE(an_srgb_format_is_decoded_first) {
    BOOST_TEST(Quad::linearises(VK_FORMAT_R8_SRGB));
    BOOST_TEST(Quad::linearises(VK_FORMAT_R8G8_SRGB));
    BOOST_TEST(Quad::linearises(VK_FORMAT_R8G8B8_SRGB));
    BOOST_TEST(Quad::linearises(VK_FORMAT_B8G8R8_SRGB));
    BOOST_TEST(Quad::linearises(VK_FORMAT_R8G8B8A8_SRGB));
    BOOST_TEST(Quad::linearises(VK_FORMAT_B8G8R8A8_SRGB));
    BOOST_TEST(Quad::linearises(VK_FORMAT_A8B8G8R8_SRGB_PACK32));
}

/**
 * A format that stores what it is given leaves a quad's colour alone. That covers the UNORM twins
 * of the formats above, a float target and VK_FORMAT_UNDEFINED.
 **/
BOOST_AUTO_TEST_CASE(any_other_format_is_left_alone) {
    BOOST_TEST(!Quad::linearises(VK_FORMAT_R8G8B8A8_UNORM));
    BOOST_TEST(!Quad::linearises(VK_FORMAT_B8G8R8A8_UNORM));
    BOOST_TEST(!Quad::linearises(VK_FORMAT_A8B8G8R8_UNORM_PACK32));
    BOOST_TEST(!Quad::linearises(VK_FORMAT_R16G16B16A16_SFLOAT));
    BOOST_TEST(!Quad::linearises(VK_FORMAT_UNDEFINED));
}

BOOST_AUTO_TEST_SUITE_END()
