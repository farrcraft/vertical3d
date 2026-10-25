/**
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 **/

#include <api/render/realtime/DeviceContext.h>
#include <api/render/realtime/Handle.h>
#include <api/render/realtime/Textures.h>
#include <api/render/realtime/vulkan/frame/Ring.h>
#include <api/render/realtime/vulkan/pipeline/Resources.h>
#include <api/render/realtime/vulkan/pipeline/Texture.h>
#include <api/render/realtime/vulkan/pipeline/TextureArray.h>

#include <vulkan/vulkan.h>

#include <cstdint>
#include <set>
#include <stdexcept>

#include <boost/shared_ptr.hpp>
#include <boost/test/unit_test.hpp>

#include "Headless.h"

using v3d::render::realtime::TextureHandle;
using v3d::render::realtime::Textures;
using v3d::render::realtime::vulkan::frame::Ring;
using v3d::render::realtime::vulkan::pipeline::TextureArray;

namespace {

const VkFormat colourFormat = VK_FORMAT_R8G8B8A8_UNORM;
const uint32_t width = 16;
const uint32_t height = 16;

/**
 * @return a texture of one colour, registered with the context's textures
 **/
TextureHandle flat(v3d::test::Headless* headless, unsigned char red) {
    const unsigned char pixel[4] = {red, 0x00, 0x00, 0xFF};
    return headless->context->textures()->texture(pixel, 1, 1, 4);
}

/**
 * @return the white texture's descriptor, which a case writes into slots it only counts
 **/
VkDescriptorImageInfo white(v3d::test::Headless* headless) {
    const Textures& textures = *headless->context->textures();
    return headless->context->resources()->texture(textures.white())->descriptor();
}

BOOST_AUTO_TEST_SUITE(texture_array_test)

/**
 * Every texture has a slot of its own, the white texture among them, and every textured draw
 * binds the one material. An unset handle, and one released, read the white texture's slot.
 **/
BOOST_AUTO_TEST_CASE(every_texture_has_a_slot_of_its_own) {
    v3d::test::Headless headless(colourFormat, width, height);
    const boost::shared_ptr<Textures> textures = headless.context->textures();

    const TextureHandle red = flat(&headless, 0xFF);
    const TextureHandle dark = flat(&headless, 0x40);
    const std::set<uint32_t> slots = {textures->slot(textures->white()), textures->slot(red), textures->slot(dark)};
    BOOST_CHECK_EQUAL(slots.size(), 3U);
    BOOST_CHECK(textures->material().valid());
    BOOST_CHECK(headless.context->resources()->material(textures->material()) != nullptr);

    BOOST_CHECK_EQUAL(textures->slot(TextureHandle()), textures->slot(textures->white()));
    BOOST_CHECK(textures->release(red));
    BOOST_CHECK_EQUAL(textures->slot(red), textures->slot(textures->white()));
    BOOST_CHECK(!textures->release(textures->white()));

    headless.context->ring()->waitIdle();
    BOOST_CHECK(headless.silent());
}

/**
 * A released slot is not given out again while a frame queued before the release may read it.
 * It comes back once framesInFlight frames after that one have begun, and the lowest free slot
 * is given out first.
 **/
BOOST_AUTO_TEST_CASE(a_released_slot_waits_for_its_frame) {
    v3d::test::Headless headless(colourFormat, width, height);
    const boost::shared_ptr<Ring> ring = headless.context->ring();
    TextureArray array(headless.device, ring);
    const VkDescriptorImageInfo image = white(&headless);
    headless.submitAndWait(ring->begin());

    const uint32_t first = array.add(image);
    const uint32_t second = array.add(image);
    BOOST_CHECK_EQUAL(first, 0U);
    BOOST_CHECK_EQUAL(second, 1U);

    array.release(first);
    BOOST_CHECK_EQUAL(array.taken(), 2U);
    BOOST_CHECK_EQUAL(array.add(image), 2U);
    for (uint32_t frame = 0; frame < ring->framesInFlight(); frame++) {
        headless.submit(ring->begin());
        BOOST_CHECK_EQUAL(array.taken(), 3U);
    }
    headless.submit(ring->begin());
    BOOST_CHECK_EQUAL(array.taken(), 2U);
    BOOST_CHECK_EQUAL(array.add(image), first);

    ring->waitIdle();
    BOOST_CHECK(headless.silent());
}

/**
 * An array with every slot taken refuses another texture, rather than overwriting one a draw
 * may read.
 **/
BOOST_AUTO_TEST_CASE(a_full_array_refuses_a_texture) {
    v3d::test::Headless headless(colourFormat, width, height);
    TextureArray array(headless.device, headless.context->ring());
    const VkDescriptorImageInfo image = white(&headless);
    BOOST_REQUIRE_GT(array.capacity(), 0U);

    for (uint32_t slot = 0; slot < array.capacity(); slot++) {
        array.add(image);
    }
    BOOST_CHECK_EQUAL(array.taken(), array.capacity());
    BOOST_CHECK_THROW(array.add(image), std::runtime_error);

    headless.context->ring()->waitIdle();
    BOOST_CHECK(headless.silent());
}

BOOST_AUTO_TEST_SUITE_END()

};  // namespace
