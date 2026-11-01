/**
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 **/

#include <api/font/Segmenter.h>
#include <api/log/Logger.h>

#include <cstddef>
#include <string_view>
#include <vector>

#include <boost/make_shared.hpp>
#include <boost/test/unit_test.hpp>

namespace {

/**
 * A segmenter with Unicode's rules untailored.
 **/
struct Segmenting {
    Segmenting() :
        segmenter("", boost::make_shared<v3d::log::Logger>()) {
        BOOST_REQUIRE(segmenter.valid());
    }

    /**
     * @return the offsets of a string's line breaks, mandatory or not
     **/
    std::vector<std::size_t> breaks(std::string_view text) const {
        std::vector<std::size_t> offsets;
        for (const v3d::font::LineBreak& found : segmenter.lineBreaks(text)) {
            offsets.push_back(found.offset);
        }
        return offsets;
    }

    v3d::font::Segmenter segmenter;
};

/**
 * Check a list of offsets against the one expected.
 **/
void same(const std::vector<std::size_t>& found, const std::vector<std::size_t>& expected) {
    BOOST_CHECK_EQUAL_COLLECTIONS(found.begin(), found.end(), expected.begin(), expected.end());
}

BOOST_FIXTURE_TEST_SUITE(segmenter_test, Segmenting)

/**
 * A letter and its combining accent are one cluster, as is an emoji sequence joined by zero
 * width joiners and a flag made of two regional indicators.
 **/
BOOST_AUTO_TEST_CASE(a_cluster_holds_what_reads_as_one_character) {
    // e and a combining acute accent, U+0301, then an exclamation mark
    const std::vector<std::size_t> accented = segmenter.graphemes("e\xCC\x81!");
    same(accented, {0, 3, 4});

    // man, zero width joiner, woman, zero width joiner, girl
    const std::vector<std::size_t> family = segmenter.graphemes("\xF0\x9F\x91\xA8\xE2\x80\x8D\xF0\x9F\x91\xA9\xE2\x80\x8D\xF0\x9F\x91\xA7");
    same(family, {0, 18});

    // the regional indicators N and Z, then a
    const std::vector<std::size_t> flag = segmenter.graphemes("\xF0\x9F\x87\xB3\xF0\x9F\x87\xBF" "a");
    same(flag, {0, 8, 9});
}

/**
 * A line may break after a hyphen, and after a space but not before a closing bracket.
 **/
BOOST_AUTO_TEST_CASE(lines_break_at_unicode_opportunities) {
    const std::vector<std::size_t> hyphen = breaks("well-known");
    same(hyphen, {5, 10});

    const std::vector<std::size_t> bracket = breaks("(see this) now");
    same(bracket, {5, 11, 14});
}

/**
 * A line has to end after a newline, and may end anywhere else it breaks.
 **/
BOOST_AUTO_TEST_CASE(a_newline_is_a_mandatory_break) {
    const std::vector<v3d::font::LineBreak> found = segmenter.lineBreaks("one two\nthree");
    BOOST_REQUIRE_EQUAL(found.size(), 3U);
    BOOST_CHECK_EQUAL(found[0].offset, 4U);
    BOOST_CHECK(!found[0].mandatory);
    BOOST_CHECK_EQUAL(found[1].offset, 8U);
    BOOST_CHECK(found[1].mandatory);
    BOOST_CHECK_EQUAL(found[2].offset, 13U);
}

/**
 * Thai marks no word boundaries, so its line breaks come from a dictionary: the two words of
 * "Thai language" can be split.
 **/
BOOST_AUTO_TEST_CASE(thai_lines_break_between_dictionary_words) {
    // phasa, then thai, three bytes to a letter
    const std::vector<std::size_t> thai = breaks("\xE0\xB8\xA0\xE0\xB8\xB2\xE0\xB8\xA9\xE0\xB8\xB2\xE0\xB9\x84\xE0\xB8\x97\xE0\xB8\xA2");
    same(thai, {12, 21});
}

/**
 * Words are told apart from the spaces and punctuation between them, and the segments cover
 * the string with no gap.
 **/
BOOST_AUTO_TEST_CASE(words_are_told_from_what_separates_them) {
    const std::vector<v3d::font::WordSegment> found = segmenter.words("Hi, you 42");
    BOOST_REQUIRE_EQUAL(found.size(), 6U);
    const std::vector<bool> expected{true, false, false, true, false, true};
    std::size_t previous = 0;
    for (std::size_t i = 0; i < found.size(); ++i) {
        BOOST_CHECK_EQUAL(found[i].start, previous);
        BOOST_CHECK_EQUAL(found[i].word, expected[i]);
        previous = found[i].end;
    }
    BOOST_CHECK_EQUAL(found[3].start, 4U);
    BOOST_CHECK_EQUAL(found[3].end, 7U);
    BOOST_CHECK_EQUAL(previous, 10U);
}

/**
 * A malformed byte is one cluster of its own, and nothing comes from an empty string but the
 * offset a caret stops at in it.
 **/
BOOST_AUTO_TEST_CASE(malformed_and_empty_strings_are_bounded) {
    const std::vector<std::size_t> malformed = segmenter.graphemes("a\xFF" "b");
    same(malformed, {0, 1, 2, 3});

    const std::vector<std::size_t> empty = segmenter.graphemes("");
    same(empty, {0});
    BOOST_CHECK(segmenter.words("").empty());
    BOOST_CHECK(segmenter.lineBreaks("").empty());
}

/**
 * A language tag is taken as the locale whose rules apply.
 **/
BOOST_AUTO_TEST_CASE(a_language_tag_names_the_locale) {
    const v3d::font::Segmenter japanese("ja-JP", boost::make_shared<v3d::log::Logger>());
    BOOST_CHECK(japanese.valid());
}

BOOST_AUTO_TEST_SUITE_END()

};  // namespace
