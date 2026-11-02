/**
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 **/

#include <api/font/glyph/Store.h>
#include <api/render/realtime/resource/Handle.h>
#include <api/ui/paint/GlyphPages.h>

#include <cstddef>
#include <cstdint>
#include <vector>

#include <boost/test/unit_test.hpp>
#include <glm/vec2.hpp>

namespace {

using v3d::render::realtime::resource::TextureHandle;

/**
 * Pages of 64 pixels cut into four plots of 32, with one page kept.
 **/
const v3d::font::glyph::Store::Spec kSmall{ .pageSize = 64, .plotSize = 32, .pages = 1, .padding = 1 };

/**
 * What the device was asked to do.
 **/
struct Device {
    std::vector<std::uint32_t> created;          /**< each texture made, by its side **/
    std::vector<glm::uvec2> updated;             /**< each region copied, by its place **/
    std::vector<unsigned char> firstTexels;      /**< the first pixel of each region copied **/
    std::vector<std::uint32_t> released;         /**< each texture released, by its id **/
};

/**
 * Pages that tell a device what they do.
 **/
struct Recording {
    Recording() :
        pages(
            [this](const unsigned char*, std::uint32_t side, std::uint32_t) {
                device.created.push_back(side);
                return TextureHandle(static_cast<std::uint32_t>(device.created.size()));
            },
            [this](const TextureHandle&, const glm::uvec2& at, const unsigned char* pixels, const glm::uvec2&, std::uint32_t) {
                device.updated.push_back(at);
                device.firstTexels.push_back(pixels[0]);
            },
            [this](const TextureHandle& texture) { device.released.push_back(texture.id()); }),
        store(kSmall) {
    }

    /**
     * Ask the store for a glyph filling a plot but for its padding, its shade its index.
     **/
    void glyph(unsigned int index) {
        store.glyph(v3d::font::glyph::Key{ .index = index, .size = 16.0F }, [](const v3d::font::glyph::Key& key) {
            return v3d::font::glyph::Bitmap{ .size = glm::uvec2(30), .pixels = std::vector<unsigned char>(std::size_t{30} * 30, static_cast<unsigned char>(key.index)) };
        });
    }

    Device device;
    v3d::ui::paint::GlyphPages pages;
    v3d::font::glyph::Store store;
};

BOOST_FIXTURE_TEST_SUITE(glyph_pages_test, Recording)

/**
 * A page the store adds becomes a texture, made whole, and a plot changed after is copied into
 * it with its own pixels. Nothing changed copies nothing.
 **/
BOOST_AUTO_TEST_CASE(pages_are_made_then_their_plots_copied) {
    glyph(1);
    pages.sync(&store);
    BOOST_REQUIRE_EQUAL(device.created.size(), 1U);
    BOOST_CHECK_EQUAL(device.created[0], 64U);
    BOOST_CHECK(device.updated.empty());
    BOOST_CHECK(pages.texture(0).valid());
    BOOST_CHECK(!pages.texture(1).valid());

    glyph(2);
    pages.sync(&store);
    BOOST_REQUIRE_EQUAL(device.updated.size(), 1U);
    BOOST_CHECK(device.updated[0] == glm::uvec2(32, 0));
    // the plot's first row and column are padding
    BOOST_CHECK_EQUAL(static_cast<int>(device.firstTexels[0]), 0);

    pages.sync(&store);
    BOOST_CHECK_EQUAL(device.updated.size(), 1U);
}

/**
 * A page added past the limit gets a texture, which is released once the store lets the page go.
 **/
BOOST_AUTO_TEST_CASE(a_page_let_go_is_released) {
    for (unsigned int index = 1; index <= 5; index++) {
        glyph(index);
    }
    pages.sync(&store);
    BOOST_CHECK_EQUAL(pages.pages(), 2U);
    store.beginFrame();
    store.beginFrame();
    pages.sync(&store);
    BOOST_CHECK_EQUAL(pages.pages(), 1U);
    BOOST_REQUIRE_EQUAL(device.released.size(), 1U);
    BOOST_CHECK_EQUAL(device.released[0], 2U);
}

BOOST_AUTO_TEST_SUITE_END()

};  // namespace
