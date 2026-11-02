/**
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 **/

#include <api/font/CurveStore.h>
#include <api/font/Curves.h>
#include <api/font/Face.h>
#include <api/font/FaceStyle.h>
#include <api/font/FallbackChain.h>
#include <api/font/Family.h>
#include <api/font/Field.h>
#include <api/font/GlyphStore.h>
#include <api/font/Library.h>
#include <api/font/Paragraph.h>
#include <api/font/Rasterize.h>
#include <api/font/ShapedRun.h>
#include <api/font/Shaper.h>
#include <api/font/ShapingCache.h>
#include <api/font/TextStyle.h>
#include <api/font/Variation.h>
#include <api/log/Logger.h>

#include <cstdint>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

#include <boost/make_shared.hpp>
#include <boost/shared_ptr.hpp>
#include <boost/test/unit_test.hpp>

namespace {

/**
 * The variable fixture's bar, which is as wide in units as its weight and 100 units narrower
 * than its advance.
 **/
constexpr std::string_view kBar = "\xEE\x80\x80";

/**
 * The size the bar is drawn at: 100 pixels to the em, so a unit is a hundredth of a pixel.
 **/
constexpr float kSize = 100.0F;

/**
 * Half of an axis's range above its default, and all of it, in 2.14 fixed point.
 **/
constexpr std::int32_t kHalf = 8192;
constexpr std::int32_t kWhole = 16384;

/**
 * The variable fixture, with a weight axis and an optical size axis, and a face with no axes.
 * tests/fonts holds them and the script that writes them.
 **/
struct Variable {
    Variable() :
        logger(boost::make_shared<v3d::log::Logger>()),
        library(boost::make_shared<v3d::font::Library>()),
        face(v3d::font::Face::open(library, "fonts/variable.ttf", logger)),
        still(v3d::font::Face::open(library, "fonts/colr0.ttf", logger)),
        chain(boost::make_shared<v3d::font::FallbackChain>()) {
        BOOST_REQUIRE(face && still);
        const boost::shared_ptr<v3d::font::Family> family = boost::make_shared<v3d::font::Family>("V3D Variable");
        family->add(face);
        chain->add(family);
    }

    /**
     * @return the bar's glyph index
     **/
    unsigned int bar() const {
        return face->glyphIndex(0xE000);
    }

    /**
     * @return the bar shaped at a weight and at the test size, with variations
     **/
    v3d::font::ShapedRun shaped(unsigned int weight, const std::vector<v3d::font::Variation>& variations = {}) const {
        const std::vector<v3d::font::ShapedRun> runs = v3d::font::Shaper(chain).shape(kBar, v3d::font::FaceStyle{ .weight = weight }, kSize, {}, variations);
        BOOST_REQUIRE_EQUAL(runs.size(), 1U);
        BOOST_REQUIRE_EQUAL(runs.front().glyphs.size(), 1U);
        return runs.front();
    }

    boost::shared_ptr<v3d::log::Logger> logger;
    boost::shared_ptr<v3d::font::Library> library;
    boost::shared_ptr<v3d::font::Face> face;
    boost::shared_ptr<v3d::font::Face> still;
    boost::shared_ptr<v3d::font::FallbackChain> chain;
};

BOOST_FIXTURE_TEST_SUITE(variation_test, Variable)

/**
 * A variable face reports its axes, and its weight axis's range as the weights it draws. A face
 * with no axes draws its own weight alone.
 **/
BOOST_AUTO_TEST_CASE(a_face_reports_its_axes) {
    const std::vector<v3d::font::FaceAxis>& axes = face->axes();
    BOOST_REQUIRE_EQUAL(axes.size(), 2U);
    BOOST_CHECK_EQUAL(axes[0].tag, "wght");
    BOOST_CHECK_EQUAL(axes[0].min, 100.0F);
    BOOST_CHECK_EQUAL(axes[0].initial, 400.0F);
    BOOST_CHECK_EQUAL(axes[0].max, 900.0F);
    BOOST_CHECK_EQUAL(axes[1].tag, "opsz");
    BOOST_CHECK_EQUAL(axes[1].min, 8.0F);
    BOOST_CHECK_EQUAL(axes[1].max, 64.0F);
    BOOST_CHECK(face->weights() == std::make_pair(100U, 900U));

    BOOST_CHECK(still->axes().empty());
    BOOST_CHECK(still->weights() == std::make_pair(400U, 400U));
}

/**
 * The weight axis takes the style's weight and the optical size axis the size, each clamped to
 * its range, and a variation is set over both. Every axis at its default is no coordinates.
 **/
BOOST_AUTO_TEST_CASE(a_style_sets_the_coordinates) {
    const v3d::font::FaceStyle regular;
    BOOST_CHECK(face->coordinates(regular, 16.0F, {}).empty());
    BOOST_CHECK(face->coordinates(v3d::font::FaceStyle{ .weight = 650 }, 16.0F, {}) == (std::vector<std::int32_t>{ kHalf, 0 }));
    BOOST_CHECK(face->coordinates(v3d::font::FaceStyle{ .weight = 250 }, 16.0F, {}) == (std::vector<std::int32_t>{ -kHalf, 0 }));
    BOOST_CHECK(face->coordinates(regular, 40.0F, {}) == (std::vector<std::int32_t>{ 0, kHalf }));
    BOOST_CHECK(face->coordinates(v3d::font::FaceStyle{ .weight = 1000 }, 200.0F, {}) == (std::vector<std::int32_t>{ kWhole, kWhole }));

    BOOST_CHECK(face->coordinates(regular, 16.0F, { { "wght", 650.0F } }) == (std::vector<std::int32_t>{ kHalf, 0 }));
    BOOST_CHECK(face->coordinates(v3d::font::FaceStyle{ .weight = 900 }, 40.0F, { { "wght", 400.0F }, { "opsz", 16.0F } }).empty());
    BOOST_CHECK(face->coordinates(regular, 16.0F, { { "wgh", 650.0F }, { "wdth", 75.0F } }).empty());
    BOOST_CHECK(still->coordinates(v3d::font::FaceStyle{ .weight = 650 }, 40.0F, { { "wght", 650.0F } }).empty());
}

/**
 * A weight between the Regular and Black instances is shaped with the advance between theirs,
 * and its run holds the coordinates it was shaped at.
 **/
BOOST_AUTO_TEST_CASE(a_weight_between_two_named_instances_is_shaped_between_them) {
    BOOST_CHECK_CLOSE(shaped(400).glyphs.front().advance, 50.0F, 0.5F);
    BOOST_CHECK_CLOSE(shaped(900).glyphs.front().advance, 100.0F, 0.5F);
    const v3d::font::ShapedRun between = shaped(650);
    BOOST_CHECK_CLOSE(between.glyphs.front().advance, 75.0F, 0.5F);
    // at 100 pixels the optical size is the axis's largest
    BOOST_CHECK(between.coordinates == (std::vector<std::int32_t>{ kHalf, kWhole }));

    BOOST_CHECK_CLOSE(shaped(650, { { "wght", 100.0F } }).glyphs.front().advance, 20.0F, 0.5F);
}

/**
 * A family's variable face has every weight its axis covers, so it is chosen over a face that
 * is only near the weight. A face with the weight exactly is chosen as it would be otherwise.
 **/
BOOST_AUTO_TEST_CASE(a_family_chooses_a_variable_face_by_its_range) {
    v3d::font::Family family("Mixed");
    family.add(still);
    family.add(face);
    BOOST_CHECK(family.face(v3d::font::FaceStyle{ .weight = 650 }) == face);
    BOOST_CHECK(family.face(v3d::font::FaceStyle{ .weight = 100 }) == face);
    BOOST_CHECK(family.face(v3d::font::FaceStyle{ .weight = 400 }) == still);
}

/**
 * An outline is read at its coordinates: the bar is as wide as the weight, and shorter at a
 * larger optical size. The curve store keeps each instance's curves apart.
 **/
BOOST_AUTO_TEST_CASE(an_outline_is_read_at_its_coordinates) {
    const v3d::font::CurveGlyph regular = v3d::font::outline(*face, bar());
    BOOST_CHECK_CLOSE(regular.max.x, 0.4F, 0.1F);
    BOOST_CHECK_CLOSE(regular.max.y, 0.8F, 0.1F);
    const v3d::font::CurveGlyph varied = v3d::font::outline(*face, bar(), { kHalf, kHalf });
    BOOST_CHECK_CLOSE(varied.max.x, 0.65F, 0.1F);
    BOOST_CHECK_CLOSE(varied.max.y, 0.7F, 0.1F);

    v3d::font::CurveStore store;
    const v3d::font::StoredCurves& atDefault = store.glyph(*face, bar());
    const v3d::font::StoredCurves& atHalf = store.glyph(*face, bar(), { kHalf, 0 });
    BOOST_CHECK_EQUAL(store.read(), 2U);
    BOOST_CHECK_CLOSE(atDefault.max.x, 0.4F, 0.1F);
    BOOST_CHECK_CLOSE(atHalf.max.x, 0.65F, 0.1F);
    store.glyph(*face, bar(), { kHalf, 0 });
    BOOST_CHECK_EQUAL(store.read(), 2U);
}

/**
 * Coverage and a distance field are made at their key's coordinates, whatever the face was
 * last loaded at. A key with none is made at the default.
 **/
BOOST_AUTO_TEST_CASE(glyphs_are_made_at_their_keys_coordinates) {
    const v3d::font::GlyphKey regular{ .face = face.get(), .index = bar(), .size = kSize };
    v3d::font::GlyphKey varied = regular;
    varied.coordinates = { kHalf, kWhole };

    const v3d::font::GlyphBitmap wide = v3d::font::rasterize(varied);
    BOOST_CHECK(wide.size.x >= 65U && wide.size.x <= 66U);
    BOOST_CHECK(wide.size.y >= 60U && wide.size.y <= 61U);
    const v3d::font::GlyphBitmap narrow = v3d::font::rasterize(regular);
    BOOST_CHECK(narrow.size.x >= 40U && narrow.size.x <= 41U);
    BOOST_CHECK(narrow.size.y >= 80U && narrow.size.y <= 81U);
    BOOST_CHECK_EQUAL(v3d::font::rasterize(varied).size.x, wide.size.x);

    const v3d::font::GlyphBitmap field = v3d::font::field(v3d::font::fieldKey(face.get(), bar(), 0.0F, varied.coordinates));
    const v3d::font::GlyphBitmap plain = v3d::font::field(v3d::font::fieldKey(face.get(), bar(), 0.0F));
    BOOST_CHECK_GT(field.size.x, plain.size.x + 10U);

    const v3d::font::GlyphKey stillKey{ .face = still.get(), .index = 1, .size = kSize, .coordinates = { kHalf, 0 } };
    BOOST_CHECK(v3d::font::rasterize(stillKey).pixels.empty());
}

/**
 * A paragraph's variations shape it again, and its line runs hold the coordinates its glyphs
 * are drawn at.
 **/
BOOST_AUTO_TEST_CASE(a_paragraph_follows_its_variations) {
    const boost::shared_ptr<v3d::font::ShapingCache> cache = boost::make_shared<v3d::font::ShapingCache>("", logger);
    v3d::font::TextStyle style{ .fonts = chain, .size = kSize };
    v3d::font::Paragraph paragraph(cache, std::string(kBar), style);
    BOOST_CHECK_CLOSE(paragraph.size().x, 50.0F, 0.5F);

    style.variations = { { "wght", 650.0F } };
    paragraph.setStyle(0, kBar.size(), style);
    BOOST_CHECK_CLOSE(paragraph.size().x, 75.0F, 0.5F);
    BOOST_REQUIRE_EQUAL(paragraph.lines().size(), 1U);
    BOOST_REQUIRE_EQUAL(paragraph.lines().front().runs.size(), 1U);
    BOOST_CHECK(paragraph.lines().front().runs.front().coordinates == (std::vector<std::int32_t>{ kHalf, kWhole }));
    BOOST_CHECK_EQUAL(cache->shaped(), 2U);
}

BOOST_AUTO_TEST_SUITE_END()

};  // namespace
