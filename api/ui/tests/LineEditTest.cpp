/**
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 **/

#include <api/font/text/Bidi.h>
#include <api/ui/input/LineEdit.h>
#include <api/ui/paint/Type.h>
#include <api/ui/tests/Typeset.h>

#include <string>

#include <boost/shared_ptr.hpp>
#include <boost/test/unit_test.hpp>
#include <glm/vec2.hpp>

namespace {

// e and a combining acute accent, U+0301, as UTF-8: one cluster of three bytes
const char* const eAccented = "e\xCC\x81";

// the flag of Japan: two regional indicators, U+1F1EF U+1F1F5, one cluster of eight bytes
const char* const flag = "\xF0\x9F\x87\xAF\xF0\x9F\x87\xB5";

// shalom, U+05E9 U+05DC U+05D5 U+05DD, read right to left, two bytes a letter
const char* const shalom = "\xD7\xA9\xD7\x9C\xD7\x95\xD7\x9D";

/**
 * @return a line holding a text with the caret at its start
 **/
v3d::ui::input::LineEdit line(const std::string& text) {
    v3d::ui::input::LineEdit made;
    made.text(text);
    made.home();
    return made;
}

/**
 * @return Inter, which kerns A against V, as a type
 **/
v3d::ui::paint::Type inter() {
    return v3d::ui::paint::Type(v3d::ui::typeset::renderer(v3d::ui::typeset::only("data/fonts/Inter-Regular.ttf"), 40.0F), 0.0F);
}

/**
 * @return the Hebrew fixture as a type
 **/
v3d::ui::paint::Type hebrew() {
    return v3d::ui::paint::Type(v3d::ui::typeset::renderer(v3d::ui::typeset::only(v3d::ui::typeset::hebrewFace), 40.0F), 0.0F);
}

BOOST_AUTO_TEST_SUITE(line_edit_test)

/**
 * The caret steps over a letter and its accent, and over an emoji sequence, as one character,
 * and a backspace takes each out whole.
 **/
BOOST_AUTO_TEST_CASE(the_caret_moves_by_grapheme_cluster) {
    v3d::ui::input::LineEdit edit = line(std::string(eAccented) + flag + "x");
    BOOST_CHECK(edit.right());
    BOOST_CHECK_EQUAL(edit.caret(), 3U);
    BOOST_CHECK(edit.right());
    BOOST_CHECK_EQUAL(edit.caret(), 11U);
    BOOST_CHECK(edit.left());
    BOOST_CHECK_EQUAL(edit.caret(), 3U);

    // a caret asked to stand inside a cluster stands at its start
    edit.caret(5);
    BOOST_CHECK_EQUAL(edit.caret(), 3U);

    edit.end();
    BOOST_CHECK(edit.backspace());
    BOOST_CHECK(edit.backspace());
    BOOST_CHECK_EQUAL(edit.text(), eAccented);
    BOOST_CHECK(edit.backspace());
    BOOST_CHECK(edit.text().empty());
}

/**
 * The caret goes with the character before it after typing or moving forwards, and with the
 * one after it after moving backwards.
 **/
BOOST_AUTO_TEST_CASE(the_caret_goes_with_the_way_it_moved) {
    v3d::ui::input::LineEdit edit = line("abc");
    BOOST_CHECK(edit.affinity() == v3d::font::text::Affinity::Downstream);
    edit.right();
    BOOST_CHECK(edit.affinity() == v3d::font::text::Affinity::Upstream);
    edit.left();
    BOOST_CHECK(edit.affinity() == v3d::font::text::Affinity::Downstream);
    edit.insert("x");
    BOOST_CHECK(edit.affinity() == v3d::font::text::Affinity::Upstream);
}

/**
 * A composition stands at the caret in the line as it is drawn, but not in its text, and
 * typing or moving the caret ends it. Laid out, it is a run with the caret after it.
 **/
BOOST_AUTO_TEST_CASE(a_composition_is_shown_at_the_caret) {
    v3d::ui::input::LineEdit edit = line("ad");
    edit.right();
    edit.compose("bc");
    BOOST_CHECK_EQUAL(edit.text(), "ad");
    BOOST_CHECK_EQUAL(edit.shown(), "abcd");

    const v3d::ui::input::LineEdit::Layout laid = edit.layout(v3d::ui::typeset::body());
    BOOST_CHECK_EQUAL(laid.text, "abcd");
    BOOST_REQUIRE_EQUAL(laid.composed.size(), 1U);
    BOOST_CHECK_CLOSE(laid.composed[0].x, v3d::ui::typeset::characterWidth, 0.1F);
    BOOST_CHECK_CLOSE(laid.composed[0].y, 3.0F * v3d::ui::typeset::characterWidth, 0.1F);
    BOOST_CHECK_CLOSE(laid.caret, 3.0F * v3d::ui::typeset::characterWidth, 0.1F);

    edit.insert("bc");
    BOOST_CHECK(edit.composition().empty());
    BOOST_CHECK_EQUAL(edit.text(), "abcd");
    edit.compose("z");
    edit.left();
    BOOST_CHECK(edit.composition().empty());
}

/**
 * Through kerned text the caret stands where the kerned glyphs meet, which is nearer than the
 * first letter's own width, and a press there lands between them.
 **/
BOOST_AUTO_TEST_CASE(the_caret_and_a_press_follow_kerning) {
    const v3d::ui::paint::Type type = inter();
    BOOST_REQUIRE(type.set());
    v3d::ui::input::LineEdit edit = line("AV");
    edit.right();
    const float kerned = edit.layout(type).caret;
    BOOST_CHECK_LT(kerned, type.width("A") - 0.5F);

    edit.home();
    edit.at(kerned + 0.5F, type);
    BOOST_CHECK_EQUAL(edit.caret(), 1U);
    edit.at(type.width("AV") + 5.0F, type);
    BOOST_CHECK_EQUAL(edit.caret(), 2U);
}

/**
 * In text read right to left the start of the line is its right edge. The caret at the start
 * stands at the right, a press near the right edge puts it at the start, and the first
 * letter's selection is at the right end.
 **/
BOOST_AUTO_TEST_CASE(right_to_left_text_starts_at_the_right) {
    const v3d::ui::paint::Type type = hebrew();
    BOOST_REQUIRE(type.set());
    const float width = type.width(shalom);
    BOOST_REQUIRE_GT(width, 0.0F);

    v3d::ui::input::LineEdit edit = line(shalom);
    BOOST_CHECK_CLOSE(edit.layout(type).caret, width, 1.0F);
    edit.end();
    BOOST_CHECK_SMALL(edit.layout(type).caret, 1.0F);

    edit.at(width - 1.0F, type);
    BOOST_CHECK_EQUAL(edit.caret(), 0U);
    edit.at(1.0F, type);
    BOOST_CHECK_EQUAL(edit.caret(), 8U);

    edit.select(0, 2);
    const v3d::ui::input::LineEdit::Layout laid = edit.layout(type);
    BOOST_REQUIRE_EQUAL(laid.selected.size(), 1U);
    BOOST_CHECK_CLOSE(laid.selected[0].y, width, 1.0F);
    BOOST_CHECK_GT(laid.selected[0].x, width * 0.5F);
}

BOOST_AUTO_TEST_SUITE_END()

};  // namespace
