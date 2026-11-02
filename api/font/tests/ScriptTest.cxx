/**
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 **/

#include <api/font/Face.h>
#include <api/font/Family.h>
#include <api/font/FallbackChain.h>
#include <api/font/Library.h>
#include <api/font/Paragraph.h>
#include <api/font/ShapedGlyph.h>
#include <api/font/ShapedRun.h>
#include <api/font/Shaper.h>
#include <api/font/ShapingCache.h>
#include <api/font/TextStyle.h>
#include <api/log/Logger.h>

#include <cstddef>
#include <string>
#include <string_view>
#include <vector>

#include <boost/make_shared.hpp>
#include <boost/shared_ptr.hpp>
#include <boost/test/unit_test.hpp>

namespace {

const float kSize = 48.0F;

/**
 * Noto Sans, then a face for each script the shared data carries, in the order the ui's text
 * renderer tries them.
 **/
struct Scripts {
    Scripts() :
        logger(boost::make_shared<v3d::log::Logger>()),
        library(boost::make_shared<v3d::font::Library>()),
        chain(boost::make_shared<v3d::font::FallbackChain>()),
        noto(add("data/fonts/NotoSans-Regular.ttf")),
        arabic(add("data/fonts/NotoSansArabic.ttf")),
        hebrew(add("data/fonts/NotoSansHebrew.ttf")),
        devanagari(add("data/fonts/NotoSansDevanagari.ttf")),
        thai(add("data/fonts/NotoSansThai.ttf")),
        cjk(add("data/fonts/NotoSansCJKsc-Regular.otf")),
        shaper(chain) {
    }

    /**
     * @return a face opened as a family of its own at the end of the chain
     **/
    boost::shared_ptr<v3d::font::Face> add(const std::string& file) {
        const boost::shared_ptr<v3d::font::Face> face = v3d::font::Face::open(library, file, logger);
        BOOST_REQUIRE(face);
        const boost::shared_ptr<v3d::font::Family> family = boost::make_shared<v3d::font::Family>(file);
        family->add(face);
        chain->add(family);
        return face;
    }

    /**
     * @return a string shaped at the test size, which has to be one run
     **/
    v3d::font::ShapedRun run(std::string_view text, std::string_view language = {}) const {
        const std::vector<v3d::font::ShapedRun> runs = shaper.shape(text, {}, kSize, {}, {}, language);
        BOOST_REQUIRE_EQUAL(runs.size(), 1U);
        return runs.front();
    }

    boost::shared_ptr<v3d::log::Logger> logger;
    boost::shared_ptr<v3d::font::Library> library;
    boost::shared_ptr<v3d::font::FallbackChain> chain;
    boost::shared_ptr<v3d::font::Face> noto;
    boost::shared_ptr<v3d::font::Face> arabic;
    boost::shared_ptr<v3d::font::Face> hebrew;
    boost::shared_ptr<v3d::font::Face> devanagari;
    boost::shared_ptr<v3d::font::Face> thai;
    boost::shared_ptr<v3d::font::Face> cjk;
    v3d::font::Shaper shaper;
};

BOOST_FIXTURE_TEST_SUITE(script_test, Scripts)

/**
 * An Arabic word is set in the Arabic face, right to left, and each letter takes its joined form:
 * beh initial, yeh medial and teh final. The face draws a letter as its dotless form and its
 * dots, and the form that advances is not the one the letter takes alone.
 **/
BOOST_AUTO_TEST_CASE(an_arabic_word_is_joined) {
    // beh, yeh, teh: "bayt", a house. Each letter is two bytes
    const std::string word = "\xD8\xA8\xD9\x8A\xD8\xAA";
    const v3d::font::ShapedRun shaped = run(word);
    BOOST_CHECK(shaped.face == arabic);
    BOOST_CHECK_EQUAL(shaped.script, "Arab");
    BOOST_CHECK(shaped.direction == v3d::font::ShapedRun::Direction::RightToLeft);
    // the glyph that advances for the letter at a byte offset
    const auto form = [](const v3d::font::ShapedRun& from, std::size_t cluster) {
        unsigned int found = 0;
        for (const v3d::font::ShapedGlyph& glyph : from.glyphs) {
            found = glyph.cluster == cluster && glyph.advance > 0.0F ? glyph.index : found;
        }
        return found;
    };
    for (std::size_t letter = 0; letter < 3; letter++) {
        const unsigned int joined = form(shaped, letter * 2);
        BOOST_CHECK_NE(joined, 0U);
        BOOST_CHECK_NE(joined, form(run(word.substr(letter * 2, 2)), 0));
    }
    // in visual order, so the last letter's glyphs come first
    BOOST_CHECK_EQUAL(shaped.glyphs.front().cluster, 4U);
}

/**
 * A Devanagari syllable whose vowel sign is written before its consonant is reordered: ka with
 * the vowel sign i draws the sign first, to the consonant's left, though it is typed after.
 **/
BOOST_AUTO_TEST_CASE(a_devanagari_syllable_is_reordered) {
    // ka, then the vowel sign i
    const v3d::font::ShapedRun shaped = run("\xE0\xA4\x95\xE0\xA4\xBF");
    BOOST_CHECK(shaped.face == devanagari);
    BOOST_CHECK_EQUAL(shaped.script, "Deva");
    BOOST_REQUIRE_EQUAL(shaped.glyphs.size(), 2U);
    const unsigned int ka = devanagari->glyphIndex(0x0915);
    BOOST_CHECK_NE(shaped.glyphs[0].index, ka);
    BOOST_CHECK_EQUAL(shaped.glyphs[1].index, ka);
}

/**
 * Hebrew and Thai are each set in their own face. A Thai vowel written above its consonant takes
 * no advance of its own.
 **/
BOOST_AUTO_TEST_CASE(hebrew_and_thai_are_set_in_their_faces) {
    // shalom
    const v3d::font::ShapedRun shalom = run("\xD7\xA9\xD7\x9C\xD7\x95\xD7\x9D");
    BOOST_CHECK(shalom.face == hebrew);
    BOOST_CHECK(shalom.direction == v3d::font::ShapedRun::Direction::RightToLeft);

    // sawatdi: so suea, wo waen, mai han akat above it, then so suea, do dek, sara i
    const v3d::font::ShapedRun sawatdi = run("\xE0\xB8\xAA\xE0\xB8\xA7\xE0\xB8\xB1\xE0\xB8\xAA\xE0\xB8\x94\xE0\xB8\xB5");
    BOOST_CHECK(sawatdi.face == thai);
    BOOST_CHECK_EQUAL(sawatdi.script, "Thai");
    BOOST_REQUIRE_EQUAL(sawatdi.glyphs.size(), 6U);
    BOOST_CHECK_EQUAL(sawatdi.glyphs[2].advance, 0.0F);
    BOOST_CHECK_GT(sawatdi.glyphs[1].advance, 0.0F);
}

/**
 * Chinese, Japanese kana and Korean Hangul are all set in the CJK face.
 **/
BOOST_AUTO_TEST_CASE(cjk_text_is_set_in_the_cjk_face) {
    for (const std::string_view text : { "\xE4\xB8\xAD\xE6\x96\x87", "\xE3\x81\x8B\xE3\x81\xAA", "\xED\x95\x9C\xEA\xB8\x80" }) {
        const v3d::font::ShapedRun shaped = run(text);
        BOOST_CHECK(shaped.face == cjk);
        BOOST_REQUIRE_EQUAL(shaped.glyphs.size(), 2U);
        BOOST_CHECK_NE(shaped.glyphs[0].index, 0U);
        BOOST_CHECK_NE(shaped.glyphs[1].index, 0U);
    }
}

/**
 * A Han character shared by Chinese and Japanese takes its Japanese form when the text is
 * Japanese, and the face's Simplified Chinese form when it is Chinese or has no language.
 **/
BOOST_AUTO_TEST_CASE(a_language_picks_the_form_of_a_shared_character) {
    // U+76F4, whose inner strokes are drawn differently in Japan and in China
    const std::string_view straight = "\xE7\x9B\xB4";
    const unsigned int chinese = run(straight, "zh-Hans").glyphs.front().index;
    BOOST_CHECK_EQUAL(run(straight).glyphs.front().index, chinese);
    BOOST_CHECK_NE(run(straight, "ja").glyphs.front().index, chinese);
}

/**
 * A paragraph is shaped in its style's language, and in the shaping cache's locale when the
 * style names none.
 **/
BOOST_AUTO_TEST_CASE(a_paragraph_is_shaped_in_its_language) {
    const std::string straight = "\xE7\x9B\xB4";
    const unsigned int japanese = run(straight, "ja").glyphs.front().index;
    const auto shaped = [&](const std::string& locale, const std::string& language) {
        v3d::font::TextStyle style{ .fonts = chain, .size = kSize };
        style.language = language;
        const v3d::font::Paragraph paragraph(boost::make_shared<v3d::font::ShapingCache>(locale, logger), straight, style);
        BOOST_REQUIRE_EQUAL(paragraph.lines().size(), 1U);
        BOOST_REQUIRE_EQUAL(paragraph.lines().front().runs.size(), 1U);
        return paragraph.lines().front().runs.front().glyphs.front().index;
    };
    BOOST_CHECK_EQUAL(shaped("", "ja"), japanese);
    BOOST_CHECK_EQUAL(shaped("ja", ""), japanese);
    BOOST_CHECK_NE(shaped("ja", "zh-Hans"), japanese);
}

BOOST_AUTO_TEST_SUITE_END()

};  // namespace
