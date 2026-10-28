/**
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 **/

#include <api/font/Utf8.h>

#include <cstddef>
#include <string>

#include <boost/test/unit_test.hpp>

namespace {

/**
 * UTF-8 decodes to one code point per character, at each sequence length.
 **/
BOOST_AUTO_TEST_CASE(utf8_decodes_one_code_point_per_character) {
    const std::u32string decoded = v3d::font::decode("a\xC3\xA9\xE2\x82\xAC\xF0\x9F\x98\x80");
    BOOST_REQUIRE_EQUAL(decoded.size(), 4U);
    BOOST_CHECK(decoded[0] == U'a');
    BOOST_CHECK(decoded[1] == static_cast<char32_t>(0xE9));
    BOOST_CHECK(decoded[2] == static_cast<char32_t>(0x20AC));
    BOOST_CHECK(decoded[3] == static_cast<char32_t>(0x1F600));
}

/**
 * Malformed UTF-8 becomes U+FFFD a byte at a time, and decoding carries on after it.
 **/
BOOST_AUTO_TEST_CASE(malformed_utf8_decodes_as_replacement_characters) {
    const char32_t replacement = 0xFFFD;

    // a lead byte with its continuation cut off
    const std::u32string truncated = v3d::font::decode("\xC3");
    BOOST_REQUIRE_EQUAL(truncated.size(), 1U);
    BOOST_CHECK(truncated[0] == replacement);

    // an overlong encoding of '/', then a character that is well formed
    const std::u32string overlong = v3d::font::decode("\xC0\xAF" "b");
    BOOST_REQUIRE_EQUAL(overlong.size(), 3U);
    BOOST_CHECK(overlong[0] == replacement);
    BOOST_CHECK(overlong[1] == replacement);
    BOOST_CHECK(overlong[2] == U'b');

    // an encoded surrogate
    const std::u32string surrogate = v3d::font::decode("\xED\xA0\x80");
    BOOST_REQUIRE_EQUAL(surrogate.size(), 3U);
    BOOST_CHECK(surrogate[0] == replacement);
}

/**
 * next() reads one character and moves the index past all of its bytes.
 **/
BOOST_AUTO_TEST_CASE(utf8_next_moves_past_the_whole_character) {
    const std::string text = "\xE2\x82\xAC" "a";
    std::size_t index = 0;
    BOOST_CHECK(v3d::font::next(text, &index) == static_cast<char32_t>(0x20AC));
    BOOST_CHECK_EQUAL(index, 3U);
    BOOST_CHECK(v3d::font::next(text, &index) == U'a');
    BOOST_CHECK_EQUAL(index, 4U);
}

};  // namespace
