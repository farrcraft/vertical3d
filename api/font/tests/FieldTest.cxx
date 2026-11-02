/**
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 **/

#include <api/font/Face.h>
#include <api/font/Field.h>
#include <api/font/GlyphStore.h>
#include <api/font/Library.h>
#include <api/font/Rasterize.h>
#include <api/font/TextStyle.h>
#include <api/log/Logger.h>

#include <cmath>
#include <cstddef>
#include <cstdint>
#include <limits>

#include <boost/make_shared.hpp>
#include <boost/shared_ptr.hpp>
#include <boost/test/unit_test.hpp>
#include <glm/vec2.hpp>

namespace {

/**
 * Inter, opened once for the suite's cases.
 **/
struct Fields {
    Fields() :
        logger(boost::make_shared<v3d::log::Logger>()),
        library(boost::make_shared<v3d::font::Library>()),
        inter(v3d::font::Face::open(library, "data/fonts/Inter-Regular.ttf", logger)) {
        BOOST_REQUIRE(inter);
    }

    /**
     * @return the key of a character's field with a spread
     **/
    v3d::font::GlyphKey key(char32_t charcode, std::uint8_t spread) const {
        return v3d::font::GlyphKey{ .face = inter.get(), .index = inter->glyphIndex(charcode), .size = v3d::font::FIELD_SIZE,
            .mode = v3d::font::GlyphMode::Distance, .spread = spread };
    }

    boost::shared_ptr<v3d::log::Logger> logger;
    boost::shared_ptr<v3d::font::Library> library;
    boost::shared_ptr<v3d::font::Face> inter;
};

/**
 * @return a pixel of a bitmap
 **/
int at(const v3d::font::GlyphBitmap& bitmap, unsigned int x, unsigned int y) {
    return bitmap.pixels[(static_cast<std::size_t>(y) * bitmap.size.x) + x];
}

BOOST_FIXTURE_TEST_SUITE(field, Fields)

BOOST_AUTO_TEST_CASE(reaches_the_spread_past_the_outline) {
    // I is one stem, so its field is its coverage grown by the spread on every side
    v3d::font::GlyphKey coverageKey = key(U'I', 0);
    coverageKey.mode = v3d::font::GlyphMode::Coverage;
    const v3d::font::GlyphBitmap coverage = v3d::font::rasterize(coverageKey);
    const v3d::font::GlyphBitmap distance = v3d::font::field(key(U'I', 8));
    BOOST_REQUIRE_GT(coverage.size.x, 0U);
    BOOST_REQUIRE_EQUAL(distance.pixels.size(), static_cast<std::size_t>(distance.size.x) * distance.size.y);
    BOOST_CHECK_LE(std::abs(static_cast<int>(distance.size.x) - static_cast<int>(coverage.size.x + 16)), 2);
    BOOST_CHECK_LE(std::abs(static_cast<int>(distance.size.y) - static_cast<int>(coverage.size.y + 16)), 2);
    BOOST_CHECK_LE(std::abs(distance.bearing.x - (coverage.bearing.x - 8)), 1);
    BOOST_CHECK_LE(std::abs(distance.bearing.y - (coverage.bearing.y - 8)), 1);
}

BOOST_AUTO_TEST_CASE(is_above_the_middle_inside_and_falls_by_the_spread) {
    const std::uint8_t spread = 8;
    const v3d::font::GlyphBitmap distance = v3d::font::field(key(U'I', spread));
    BOOST_REQUIRE_GT(distance.size.x, 0U);
    const unsigned int row = distance.size.y / 2;
    const unsigned int middle = distance.size.x / 2;
    BOOST_CHECK_GT(at(distance, middle, row), 128);
    BOOST_CHECK_EQUAL(at(distance, 0, 0), 0);

    // the first pixel outside the stem, then one pixel further out, is a pixel's distance less
    unsigned int outside = middle;
    while (outside > 0 && at(distance, outside, row) >= 128) {
        outside--;
    }
    BOOST_REQUIRE_GT(outside, 1U);
    const int fall = at(distance, outside, row) - at(distance, outside - 1, row);
    BOOST_CHECK_LE(std::abs(fall - (128 / spread)), 3);
}

BOOST_AUTO_TEST_CASE(another_mode_or_no_spread_makes_nothing) {
    v3d::font::GlyphKey coverage = key(U'I', 8);
    coverage.mode = v3d::font::GlyphMode::Coverage;
    BOOST_CHECK(v3d::font::field(coverage).pixels.empty());
    BOOST_CHECK(v3d::font::field(key(U'I', 0)).pixels.empty());
    BOOST_CHECK(v3d::font::field(v3d::font::GlyphKey{ .mode = v3d::font::GlyphMode::Distance, .spread = 8 }).pixels.empty());
}

BOOST_AUTO_TEST_CASE(a_spread_past_the_widest_is_the_widest) {
    const v3d::font::GlyphBitmap widest = v3d::font::field(key(U'I', v3d::font::MAX_SPREAD));
    const v3d::font::GlyphBitmap past = v3d::font::field(key(U'I', 64));
    BOOST_CHECK(widest.size == past.size);
}

BOOST_AUTO_TEST_CASE(a_spread_covers_the_reach_in_steps) {
    BOOST_CHECK_EQUAL(v3d::font::fieldSpread(0.0F), 4);
    BOOST_CHECK_EQUAL(v3d::font::fieldSpread(std::numeric_limits<float>::quiet_NaN()), 4);
    // a pixel of the field, and a pixel more for the effect's edge
    BOOST_CHECK_EQUAL(v3d::font::fieldSpread(1.0F / v3d::font::FIELD_SIZE), 4);
    BOOST_CHECK_EQUAL(v3d::font::fieldSpread(0.2F), 16);
    BOOST_CHECK_EQUAL(v3d::font::fieldSpread(10.0F), v3d::font::MAX_SPREAD);

    const v3d::font::GlyphKey made = v3d::font::fieldKey(inter.get(), 5, 0.2F);
    BOOST_CHECK(made.mode == v3d::font::GlyphMode::Distance);
    BOOST_CHECK_EQUAL(made.size, v3d::font::FIELD_SIZE);
    BOOST_CHECK_EQUAL(made.spread, 16);
    BOOST_CHECK_EQUAL(made.index, 5U);
}

BOOST_AUTO_TEST_CASE(an_effect_reaches_past_its_width_by_its_softness) {
    BOOST_CHECK_CLOSE(v3d::font::effectReach(v3d::font::TextEffect{ .width = 2.0F, .softness = 1.0F }, 16.0F), 3.0F / 16.0F, 0.001);
    BOOST_CHECK_CLOSE(v3d::font::effectReach(v3d::font::TextEffect{ .width = -1.0F }, 16.0F), 1.0F / 16.0F, 0.001);
    BOOST_CHECK_EQUAL(v3d::font::effectReach(v3d::font::TextEffect{ .width = 2.0F }, 0.0F), 0.0F);
}

BOOST_AUTO_TEST_CASE(an_effect_ramps_between_field_values) {
    // at 16 pixels with a spread of 16, a pixel of the text is an eighth of the field's values
    const glm::vec2 outline = v3d::font::effectRamp(v3d::font::TextEffect{ .width = 2.0F }, 16.0F, 16);
    BOOST_CHECK_CLOSE(outline.x, 0.25F, 0.001);
    BOOST_CHECK_CLOSE(outline.y, 0.25F, 0.001);
    const glm::vec2 glow = v3d::font::effectRamp(v3d::font::TextEffect{ .width = 1.0F, .softness = 2.0F }, 16.0F, 16);
    BOOST_CHECK_CLOSE(glow.x, 0.375F, 0.001);
    BOOST_CHECK_CLOSE(glow.y, 0.125F, 0.001);
    const glm::vec2 inside = v3d::font::effectRamp(v3d::font::TextEffect{ .width = -1.0F }, 16.0F, 16);
    BOOST_CHECK_CLOSE(inside.x, 0.625F, 0.001);
    // a reach past the spread is cut at the field's last value
    const glm::vec2 cut = v3d::font::effectRamp(v3d::font::TextEffect{ .width = 10.0F }, 16.0F, 16);
    BOOST_CHECK_EQUAL(cut.x, 0.0F);
}

BOOST_AUTO_TEST_SUITE_END()

};  // namespace
