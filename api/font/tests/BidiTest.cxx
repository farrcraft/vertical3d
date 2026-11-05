/**
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 **/

#include <api/font/text/Bidi.h>

#include <cstddef>
#include <cstdint>
#include <string>
#include <string_view>
#include <vector>

#include <boost/test/unit_test.hpp>

namespace {

/**
 * "I said ", shalom in Hebrew, then " today": the Hebrew is bytes 7 to 15.
 **/
constexpr std::string_view kHebrewInEnglish = "I said \xD7\xA9\xD7\x9C\xD7\x95\xD7\x9D today";

/**
 * Check runs against the starts, ends and levels expected.
 **/
void same(const std::vector<v3d::font::text::LevelRun>& found, const std::vector<v3d::font::text::LevelRun>& expected) {
    BOOST_REQUIRE_EQUAL(found.size(), expected.size());
    for (std::size_t run = 0; run < found.size(); ++run) {
        BOOST_CHECK_EQUAL(found[run].start, expected[run].start);
        BOOST_CHECK_EQUAL(found[run].end, expected[run].end);
        BOOST_CHECK_EQUAL(static_cast<int>(found[run].level), static_cast<int>(expected[run].level));
    }
}

/**
 * Hebrew in an English sentence is one run at level 1, and the line keeps the text's order
 * because the paragraph reads left to right.
 **/
BOOST_AUTO_TEST_CASE(hebrew_in_english_is_its_own_level) {
    const v3d::font::text::Bidi bidi(kHebrewInEnglish, v3d::font::text::BaseDirection::Auto);
    BOOST_CHECK(!bidi.rightToLeft(0));
    same(bidi.runs(), {{0, 7, 0}, {7, 15, 1}, {15, 21, 0}});
    BOOST_CHECK(bidi.runs()[1].rightToLeft());
    same(bidi.line(0, kHebrewInEnglish.size()), {{0, 7, 0}, {7, 15, 1}, {15, 21, 0}});
}

/**
 * A number in an Arabic sentence reads left to right at level 2, and the line draws the
 * sentence's last word first.
 **/
BOOST_AUTO_TEST_CASE(a_number_in_arabic_reads_left_to_right) {
    // adad, a space, 123, a space, huna
    const std::string text = "\xD8\xB9\xD8\xAF\xD8\xAF 123 \xD9\x87\xD9\x86\xD8\xA7";
    const v3d::font::text::Bidi bidi(text, v3d::font::text::BaseDirection::Auto);
    BOOST_CHECK(bidi.rightToLeft(0));
    same(bidi.runs(), {{0, 7, 1}, {7, 10, 2}, {10, 17, 1}});
    BOOST_CHECK_EQUAL(static_cast<int>(bidi.level(8)), 2);
    same(bidi.line(0, text.size()), {{10, 17, 1}, {7, 10, 2}, {0, 7, 1}});
}

/**
 * At each edge of the Hebrew, a caret going with the character before it and one going with
 * the character after it are drawn against different characters.
 **/
BOOST_AUTO_TEST_CASE(a_caret_at_a_boundary_follows_its_affinity) {
    const v3d::font::text::Bidi bidi(kHebrewInEnglish, v3d::font::text::BaseDirection::Auto);
    using v3d::font::text::Affinity;

    // after the space: the space's right edge, or the right edge where shin starts
    BOOST_CHECK_EQUAL(bidi.caret(7, Affinity::Upstream).character, 6U);
    BOOST_CHECK(bidi.caret(7, Affinity::Upstream).right);
    BOOST_CHECK_EQUAL(bidi.caret(7, Affinity::Downstream).character, 7U);
    BOOST_CHECK(bidi.caret(7, Affinity::Downstream).right);

    // after final mem: the left edge where mem ends, or the space's left edge
    BOOST_CHECK_EQUAL(bidi.caret(15, Affinity::Upstream).character, 13U);
    BOOST_CHECK(!bidi.caret(15, Affinity::Upstream).right);
    BOOST_CHECK_EQUAL(bidi.caret(15, Affinity::Downstream).character, 15U);
    BOOST_CHECK(!bidi.caret(15, Affinity::Downstream).right);

    // at either end there is only one character to go with
    BOOST_CHECK_EQUAL(bidi.caret(0, Affinity::Upstream).character, 0U);
    BOOST_CHECK(!bidi.caret(0, Affinity::Upstream).right);
    BOOST_CHECK_EQUAL(bidi.caret(kHebrewInEnglish.size(), Affinity::Downstream).character, 20U);
    BOOST_CHECK(bidi.caret(kHebrewInEnglish.size(), Affinity::Downstream).right);

    // an empty paragraph read right to left starts at its right edge
    BOOST_CHECK(v3d::font::text::Bidi("", v3d::font::text::BaseDirection::RightToLeft).caret(0, Affinity::Upstream).right);
}

/**
 * English in a paragraph read right to left is at level 2. A space inside it is too, until a
 * line ends at that space, which then takes the paragraph's level and is drawn at its left.
 **/
BOOST_AUTO_TEST_CASE(trailing_space_takes_the_paragraph_level) {
    const v3d::font::text::Bidi bidi("abc def", v3d::font::text::BaseDirection::RightToLeft);
    same(bidi.runs(), {{0, 7, 2}});
    same(bidi.line(0, 4), {{3, 4, 1}, {0, 3, 2}});
}

/**
 * Each paragraph takes its own direction, a line cannot cross from one to the next, and an
 * empty or out of range line has no runs.
 **/
BOOST_AUTO_TEST_CASE(paragraphs_are_resolved_apart) {
    // abc, a newline, then alef bet
    const std::string text = "abc\n\xD7\x90\xD7\x91";
    const v3d::font::text::Bidi bidi(text, v3d::font::text::BaseDirection::Auto);
    BOOST_CHECK(!bidi.rightToLeft(0));
    BOOST_CHECK(bidi.rightToLeft(5));
    BOOST_CHECK_EQUAL(bidi.line(0, 4).size(), 1U);
    BOOST_CHECK(bidi.line(0, text.size()).empty());
    BOOST_CHECK(bidi.line(2, 2).empty());
    BOOST_CHECK(bidi.line(0, text.size() + 1).empty());
    BOOST_CHECK(v3d::font::text::Bidi("", v3d::font::text::BaseDirection::Auto).runs().empty());
}

};  // namespace
