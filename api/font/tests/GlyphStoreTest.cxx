/**
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 **/

#include <api/font/glyph/Store.h>

#include <cstddef>
#include <cstdint>
#include <optional>
#include <vector>

#include <boost/test/unit_test.hpp>
#include <glm/vec2.hpp>

namespace {

/**
 * Pages of 64 pixels cut into four plots of 32, with one page kept.
 **/
const v3d::font::glyph::Store::Spec kSmall{ .pageSize = 64, .plotSize = 32, .pages = 1, .padding = 1 };

/**
 * @return a glyph of one shade filling a square of a size, its shade the key's index
 **/
v3d::font::glyph::Bitmap square(const v3d::font::glyph::Key& key, std::uint32_t side) {
    return v3d::font::glyph::Bitmap{ .size = glm::uvec2(side), .bearing = glm::ivec2(1, -static_cast<int>(side)),
        .pixels = std::vector<unsigned char>(static_cast<std::size_t>(side) * side, static_cast<unsigned char>(key.index)) };
}

/**
 * @return a glyph that fills a plot of the small store but for its padding
 **/
v3d::font::glyph::Bitmap large(const v3d::font::glyph::Key& key) {
    return square(key, 30);
}

/**
 * @return the key of a glyph by its index
 **/
v3d::font::glyph::Key keyOf(unsigned int index) {
    return v3d::font::glyph::Key{ .index = index, .size = 16.0F };
}

BOOST_AUTO_TEST_SUITE(glyph_store_test)

/**
 * A glyph is made the first time it is asked for and found after, in the same place, with its
 * pixels in the page. A different size or step of a pixel is another glyph.
 **/
BOOST_AUTO_TEST_CASE(a_glyph_is_made_once) {
    v3d::font::glyph::Store store(kSmall);
    const auto small = [](const v3d::font::glyph::Key& key) { return square(key, 4); };
    const std::optional<v3d::font::glyph::StoredGlyph> found = store.glyph(keyOf(7), small);
    const std::optional<v3d::font::glyph::StoredGlyph> again = store.glyph(keyOf(7), small);
    BOOST_REQUIRE(found.has_value() && again.has_value());
    const v3d::font::glyph::StoredGlyph first = found.value_or(v3d::font::glyph::StoredGlyph());
    BOOST_CHECK_EQUAL(store.made(), 1U);
    BOOST_CHECK(first.position == again.value_or(v3d::font::glyph::StoredGlyph()).position);
    BOOST_CHECK(first.size == glm::uvec2(4));
    BOOST_CHECK_EQUAL(static_cast<int>(store.pixels(first.page)[(first.position.y * 64) + first.position.x]), 7);

    v3d::font::glyph::Key stepped = keyOf(7);
    stepped.subpixel = 2;
    store.glyph(stepped, small);
    v3d::font::glyph::Key larger = keyOf(7);
    larger.size = 17.0F;
    store.glyph(larger, small);
    BOOST_CHECK_EQUAL(store.made(), 3U);
}

/**
 * Glyphs that fill every plot leave the next to empty the plot used longest ago, whose glyphs
 * are made again when asked for, and the page limit holds.
 **/
BOOST_AUTO_TEST_CASE(a_full_store_empties_the_plot_used_longest_ago) {
    v3d::font::glyph::Store store(kSmall);
    for (unsigned int index = 1; index <= 4; index++) {
        BOOST_REQUIRE(store.glyph(keyOf(index), large));
        store.beginFrame();
    }
    BOOST_CHECK_EQUAL(store.pages(), 1U);
    const std::optional<v3d::font::glyph::StoredGlyph> fifth = store.glyph(keyOf(5), large);
    BOOST_REQUIRE(fifth.has_value());
    BOOST_CHECK_EQUAL(store.pages(), 1U);
    BOOST_CHECK_EQUAL(store.made(), 5U);
    // the first glyph's plot was used longest ago, so the fifth took it
    BOOST_CHECK(fifth.value_or(v3d::font::glyph::StoredGlyph()).position == glm::uvec2(1, 1));
    store.glyph(keyOf(2), large);
    BOOST_CHECK_EQUAL(store.made(), 5U);
    store.beginFrame();
    store.glyph(keyOf(1), large);
    BOOST_CHECK_EQUAL(store.made(), 6U);
}

/**
 * A plot the frame being built uses is never emptied: a glyph that finds every plot in use
 * adds a page past the limit, which goes once a frame has used nothing in it.
 **/
BOOST_AUTO_TEST_CASE(a_glyph_drawn_this_frame_is_kept) {
    v3d::font::glyph::Store store(kSmall);
    for (unsigned int index = 1; index <= 5; index++) {
        BOOST_REQUIRE(store.glyph(keyOf(index), large));
    }
    BOOST_CHECK_EQUAL(store.pages(), 2U);
    BOOST_CHECK_EQUAL(store.made(), 5U);
    store.glyph(keyOf(1), large);
    BOOST_CHECK_EQUAL(store.made(), 5U);

    store.beginFrame();
    store.glyph(keyOf(1), large);
    BOOST_CHECK_EQUAL(store.pages(), 2U);
    store.beginFrame();
    BOOST_CHECK_EQUAL(store.pages(), 1U);
    // the glyph on the page let go is made again
    store.glyph(keyOf(5), large);
    BOOST_CHECK_EQUAL(store.made(), 6U);
}

/**
 * A new page is changed whole, a glyph changes its plot, and a region is given once.
 **/
BOOST_AUTO_TEST_CASE(changed_plots_are_given_once) {
    v3d::font::glyph::Store store(kSmall);
    store.glyph(keyOf(1), large);
    BOOST_CHECK_EQUAL(store.regions().size(), 4U);
    BOOST_CHECK(store.regions().empty());
    store.glyph(keyOf(2), large);
    const std::vector<v3d::font::glyph::Region> changed = store.regions();
    BOOST_REQUIRE_EQUAL(changed.size(), 1U);
    BOOST_CHECK(changed[0].position == glm::uvec2(32, 0));
    BOOST_CHECK(changed[0].size == glm::uvec2(32));
}

/**
 * A glyph larger than a plot, or whose pixels do not match its size, is not stored, and one
 * with no pixels takes no room.
 **/
BOOST_AUTO_TEST_CASE(a_glyph_that_cannot_fit_is_not_stored) {
    v3d::font::glyph::Store store(kSmall);
    BOOST_CHECK(!store.glyph(keyOf(1), [](const v3d::font::glyph::Key& key) { return square(key, 31); }));
    BOOST_CHECK(!store.glyph(keyOf(2), [](const v3d::font::glyph::Key&) {
        return v3d::font::glyph::Bitmap{ .size = glm::uvec2(4), .pixels = std::vector<unsigned char>(3) };
    }));
    const std::optional<v3d::font::glyph::StoredGlyph> space = store.glyph(keyOf(3), [](const v3d::font::glyph::Key&) { return v3d::font::glyph::Bitmap(); });
    BOOST_REQUIRE(space.has_value());
    BOOST_CHECK(space.value_or(v3d::font::glyph::StoredGlyph{ .size = glm::uvec2(1) }).size == glm::uvec2(0));
}

/**
 * A store of colour pages keeps four bytes a pixel, each glyph's rows where its pixels went, and
 * takes no glyph of one byte a pixel.
 **/
BOOST_AUTO_TEST_CASE(a_colour_store_keeps_four_bytes_a_pixel) {
    v3d::font::glyph::Store::Spec spec = kSmall;
    spec.channels = 4;
    v3d::font::glyph::Store store(spec);
    BOOST_CHECK(!store.glyph(keyOf(1), large));
    const std::optional<v3d::font::glyph::StoredGlyph> stored = store.glyph(keyOf(2), [](const v3d::font::glyph::Key&) {
        std::vector<unsigned char> pixels;
        for (int pixel = 0; pixel < 4; pixel++) {
            pixels.insert(pixels.end(), { 10, 20, 30, static_cast<unsigned char>(40 + pixel) });
        }
        return v3d::font::glyph::Bitmap{ .size = glm::uvec2(2), .pixels = pixels, .channels = 4 };
    });
    BOOST_REQUIRE(stored.has_value());
    const v3d::font::glyph::StoredGlyph at = stored.value_or(v3d::font::glyph::StoredGlyph());
    const std::vector<unsigned char>& page = store.pixels(at.page);
    BOOST_REQUIRE_EQUAL(page.size(), static_cast<std::size_t>(64) * 64 * 4);
    // the second row's first pixel, the glyph's third
    const std::size_t second = ((static_cast<std::size_t>(at.position.y + 1) * 64) + at.position.x) * 4;
    BOOST_CHECK_EQUAL(page[second], 10);
    BOOST_CHECK_EQUAL(page[second + 2], 30);
    BOOST_CHECK_EQUAL(page[second + 3], 42);
}

BOOST_AUTO_TEST_SUITE_END()

};  // namespace
