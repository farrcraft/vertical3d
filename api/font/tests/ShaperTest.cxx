/**
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 **/

#include <api/font/Bidi.h>
#include <api/font/Face.h>
#include <api/font/Family.h>
#include <api/font/FallbackChain.h>
#include <api/font/Feature.h>
#include <api/font/Library.h>
#include <api/font/ShapedGlyph.h>
#include <api/font/ShapedRun.h>
#include <api/font/Shaper.h>
#include <api/log/Logger.h>

#include <string>
#include <string_view>
#include <vector>

#include <boost/make_shared.hpp>
#include <boost/shared_ptr.hpp>
#include <boost/test/unit_test.hpp>

namespace {

const float kSize = 48.0F;

/**
 * A shaper over Inter, falling back to Noto Sans.
 **/
struct Shaping {
    Shaping() :
        logger(boost::make_shared<v3d::log::Logger>()),
        library(boost::make_shared<v3d::font::Library>()),
        inter(v3d::font::Face::open(library, "data/fonts/Inter-Regular.ttf", logger)),
        noto(v3d::font::Face::open(library, "data/fonts/NotoSans-Regular.ttf", logger)),
        chain(boost::make_shared<v3d::font::FallbackChain>()),
        shaper(chain) {
        BOOST_REQUIRE(inter && noto);
        const boost::shared_ptr<v3d::font::Family> primary = boost::make_shared<v3d::font::Family>("Inter");
        primary->add(inter);
        const boost::shared_ptr<v3d::font::Family> fallback = boost::make_shared<v3d::font::Family>("Noto Sans");
        fallback->add(noto);
        chain->add(primary);
        chain->add(fallback);
    }

    /**
     * @return a string's runs at the test size
     **/
    std::vector<v3d::font::ShapedRun> runs(std::string_view text, const std::vector<v3d::font::Feature>& features = {}) const {
        return shaper.shape(text, {}, kSize, features);
    }

    /**
     * @return how far a string advances, which has to be one run
     **/
    float advance(std::string_view text, const std::vector<v3d::font::Feature>& features = {}) const {
        const std::vector<v3d::font::ShapedRun> shaped = runs(text, features);
        BOOST_REQUIRE_EQUAL(shaped.size(), 1U);
        return shaped.empty() ? 0.0F : shaped.front().advance;
    }

    /**
     * @return the glyph indices a string shapes to, which has to be one run
     **/
    std::vector<unsigned int> indices(std::string_view text, const std::vector<v3d::font::Feature>& features = {}) const {
        std::vector<unsigned int> found;
        for (const v3d::font::ShapedRun& run : runs(text, features)) {
            for (const v3d::font::ShapedGlyph& glyph : run.glyphs) {
                found.push_back(glyph.index);
            }
        }
        return found;
    }

    boost::shared_ptr<v3d::log::Logger> logger;
    boost::shared_ptr<v3d::font::Library> library;
    boost::shared_ptr<v3d::font::Face> inter;
    boost::shared_ptr<v3d::font::Face> noto;
    boost::shared_ptr<v3d::font::FallbackChain> chain;
    v3d::font::Shaper shaper;
};

BOOST_FIXTURE_TEST_SUITE(shaper_test, Shaping)

/**
 * A kerned pair is closer together than its two letters shaped apart, and turning kerning off
 * puts them back.
 **/
BOOST_AUTO_TEST_CASE(a_pair_is_kerned) {
    const float apart = advance("A") + advance("V");
    BOOST_CHECK(advance("AV") < apart - 1.0F);
    BOOST_CHECK_CLOSE(advance("AV", { { .tag = "kern", .value = 0 } }), apart, 0.01F);
}

/**
 * Tabular figures give every digit one width, where Inter's own figures differ.
 **/
BOOST_AUTO_TEST_CASE(tabular_figures_are_one_width) {
    BOOST_CHECK(advance("1") < advance("8"));
    const std::vector<v3d::font::Feature> tabular{ { .tag = "tnum" } };
    BOOST_CHECK_CLOSE(advance("1", tabular), advance("8", tabular), 0.01F);
    // a tag that is not four letters is left out
    BOOST_CHECK_CLOSE(advance("1", { { .tag = "tnum!" } }), advance("1"), 0.01F);
}

/**
 * A combining mark takes no room of its own and stays in its base's run, and each glyph
 * names the byte its character starts at.
 **/
BOOST_AUTO_TEST_CASE(a_mark_sits_on_its_base) {
    // x and a combining acute accent, U+0301, as UTF-8
    const std::vector<v3d::font::ShapedRun> shaped = runs("x\xCC\x81");
    BOOST_REQUIRE_EQUAL(shaped.size(), 1U);
    BOOST_REQUIRE_EQUAL(shaped.front().glyphs.size(), 2U);
    BOOST_CHECK_CLOSE(shaped.front().advance, advance("x"), 0.01F);
    BOOST_CHECK_EQUAL(shaped.front().glyphs[1].advance, 0.0F);
    BOOST_CHECK_EQUAL(shaped.front().glyphs[0].cluster, 0U);
    BOOST_CHECK_EQUAL(shaped.front().glyphs[1].cluster, 1U);
}

/**
 * Inter's contextual alternates change the glyphs of a sequence, which turning them off
 * leaves as they are.
 **/
BOOST_AUTO_TEST_CASE(contextual_alternates_are_applied) {
    BOOST_CHECK(indices("->") != indices("->", { { .tag = "calt", .value = 0 } }));
}

/**
 * A string splits into runs where its face or its script changes, a space stays in the run
 * before it, and Arabic is shaped right to left.
 **/
BOOST_AUTO_TEST_CASE(runs_split_by_face_and_script) {
    // a, a space, a capital DZ with a caron which only Noto Sans has, a space, then two Arabic
    // letters, which neither face has
    const std::string text = "a \xC7\x84 \xD8\xA7\xD9\x84";
    const std::vector<v3d::font::ShapedRun> shaped = runs(text);
    BOOST_REQUIRE_EQUAL(shaped.size(), 3U);
    BOOST_CHECK(shaped[0].face == inter);
    BOOST_CHECK_EQUAL(shaped[0].end, 2U);
    BOOST_CHECK(shaped[1].face == noto);
    BOOST_CHECK_EQUAL(shaped[1].script, "Latn");
    BOOST_CHECK_EQUAL(shaped[2].script, "Arab");
    BOOST_CHECK(shaped[2].direction == v3d::font::ShapedRun::Direction::RightToLeft);
    BOOST_CHECK(shaped[2].face == inter);
    BOOST_CHECK_EQUAL(shaped[2].end, text.size());
}

/**
 * A number in an Arabic sentence is a run of its own, shaped left to right at a higher level
 * than the Arabic either side of it.
 **/
BOOST_AUTO_TEST_CASE(a_number_in_arabic_is_shaped_left_to_right) {
    // adad, a space, 123, a space, huna
    const std::vector<v3d::font::ShapedRun> shaped = runs("\xD8\xB9\xD8\xAF\xD8\xAF 123 \xD9\x87\xD9\x86\xD8\xA7");
    BOOST_REQUIRE_EQUAL(shaped.size(), 3U);
    BOOST_CHECK(shaped[0].direction == v3d::font::ShapedRun::Direction::RightToLeft);
    BOOST_CHECK_EQUAL(shaped[1].start, 7U);
    BOOST_CHECK_EQUAL(shaped[1].end, 10U);
    BOOST_CHECK(shaped[1].direction == v3d::font::ShapedRun::Direction::LeftToRight);
    BOOST_CHECK_EQUAL(static_cast<int>(shaped[1].level), 2);
    BOOST_CHECK(shaped[2].direction == v3d::font::ShapedRun::Direction::RightToLeft);
    // in a paragraph read right to left, Latin is at level 2
    const std::vector<v3d::font::ShapedRun> forced = shaper.shape("abc", {}, kSize, {}, {}, v3d::font::BaseDirection::RightToLeft);
    BOOST_REQUIRE_EQUAL(forced.size(), 1U);
    BOOST_CHECK_EQUAL(static_cast<int>(forced.front().level), 2);
}

/**
 * Nothing is shaped from an empty string or at a size out of range.
 **/
BOOST_AUTO_TEST_CASE(nothing_is_shaped_out_of_range) {
    BOOST_CHECK(runs("").empty());
    BOOST_CHECK(shaper.shape("a", {}, 0.0F).empty());
    BOOST_CHECK(shaper.shape("a", {}, v3d::font::Shaper::LARGEST * 2.0F).empty());
    BOOST_CHECK(v3d::font::Shaper(boost::make_shared<v3d::font::FallbackChain>()).shape("a", {}, kSize).empty());
}

BOOST_AUTO_TEST_SUITE_END()

};  // namespace
