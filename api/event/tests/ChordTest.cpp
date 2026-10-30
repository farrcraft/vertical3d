/**
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 **/

#include <api/event/Chord.h>

#include <boost/test/unit_test.hpp>

namespace {

BOOST_AUTO_TEST_SUITE(chord_test)

/**
 * Modifier words before the key make a chord, in any order.
 **/
BOOST_AUTO_TEST_CASE(modifier_words_before_the_key_make_a_chord) {
    const v3d::event::Chord undo = v3d::event::chord("ctrl+z");
    BOOST_TEST(undo.key == "z");
    BOOST_TEST(undo.modifiers.ctrl);
    BOOST_TEST(!undo.modifiers.shift);
    BOOST_TEST(!undo.modifiers.alt);

    const v3d::event::Chord redo = v3d::event::chord("shift+ctrl+z");
    BOOST_TEST(redo.key == "z");
    BOOST_CHECK(redo.modifiers == (v3d::event::Modifiers{.ctrl = true, .shift = true, .alt = false}));

    const v3d::event::Chord click = v3d::event::chord("alt+left");
    BOOST_TEST(click.key == "left");
    BOOST_TEST(click.modifiers.alt);
}

/**
 * A name with no modifier words is a key alone, and so is one with a word that is not a
 * modifier, which then names a key no device sends.
 **/
BOOST_AUTO_TEST_CASE(anything_else_is_a_key_alone) {
    BOOST_TEST(v3d::event::chord("left_control").key == "left_control");
    BOOST_TEST(v3d::event::chord("left_control").modifiers.count() == 0);
    BOOST_TEST(v3d::event::chord("meta+z").key == "meta+z");
    BOOST_TEST(v3d::event::chord("meta+z").modifiers.count() == 0);
    BOOST_TEST(v3d::event::chord("ctrl+").key == "ctrl+");
    BOOST_TEST(v3d::event::chord("ctrl++z").key == "ctrl++z");
    BOOST_TEST(v3d::event::chord("+z").key == "+z");
}

/**
 * A chord's modifiers are within those held when every one it names is held.
 **/
BOOST_AUTO_TEST_CASE(modifiers_are_within_those_held) {
    const v3d::event::Modifiers ctrl{.ctrl = true, .shift = false, .alt = false};
    const v3d::event::Modifiers both{.ctrl = true, .shift = true, .alt = false};
    BOOST_TEST(v3d::event::Modifiers().within(ctrl));
    BOOST_TEST(ctrl.within(both));
    BOOST_TEST(!both.within(ctrl));
    BOOST_TEST(both.count() == 2);
}

BOOST_AUTO_TEST_SUITE_END()

};  // namespace
