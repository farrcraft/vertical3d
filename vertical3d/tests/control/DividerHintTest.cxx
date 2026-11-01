/**
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 **/

#include <vertical3d/src/control/DividerHint.h>

#include <boost/test/unit_test.hpp>

namespace {

using v3d::editor::control::DividerHint;

BOOST_AUTO_TEST_SUITE(divider_hint_test)

/**
 * The hint shows once a divider has been lit for the delay, and not before.
 **/
BOOST_AUTO_TEST_CASE(the_hint_waits_while_a_divider_is_lit) {
    DividerHint hint;
    BOOST_CHECK(!hint.due({ .lit = false, .held = false }, 1.0F));
    BOOST_CHECK(!hint.due({ .lit = true, .held = false }, 2.0F));
    BOOST_CHECK(!hint.due({ .lit = true, .held = false }, 2.0F + (DividerHint::DELAY / 2.0F)));
    BOOST_CHECK(hint.due({ .lit = true, .held = false }, 2.0F + DividerHint::DELAY));
}

/**
 * Holding a divider hides the hint, and so does the cursor leaving it, which starts the wait
 * again.
 **/
BOOST_AUTO_TEST_CASE(holding_or_leaving_a_divider_ends_the_hint) {
    DividerHint hint;
    BOOST_CHECK(!hint.due({ .lit = true, .held = false }, 0.0F));
    BOOST_CHECK(hint.due({ .lit = true, .held = false }, 1.0F));
    BOOST_CHECK(!hint.due({ .lit = true, .held = true }, 1.5F));
    BOOST_CHECK(!hint.due({ .lit = true, .held = false }, 2.0F));
    BOOST_CHECK(hint.due({ .lit = true, .held = false }, 2.0F + DividerHint::DELAY));
    BOOST_CHECK(!hint.due({ .lit = false, .held = false }, 3.0F));
    BOOST_CHECK(!hint.due({ .lit = true, .held = false }, 3.1F));
}

BOOST_AUTO_TEST_SUITE_END()

};  // namespace
