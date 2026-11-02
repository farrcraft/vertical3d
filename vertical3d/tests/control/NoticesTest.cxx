/**
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 **/

#include <vertical3d/src/control/chrome/Notices.h>

#include <optional>
#include <string>

#include <boost/test/unit_test.hpp>

namespace {

using v3d::editor::control::chrome::Notices;

constexpr float CLOSE = 1e-3F;

BOOST_AUTO_TEST_SUITE(notices_test)

/**
 * A notice starts when it is first drawn, fades in, stays, and fades out at the end of its time,
 * after which nothing shows.
 **/
BOOST_AUTO_TEST_CASE(a_notice_fades_in_stays_and_fades_out) {
    Notices notices;
    BOOST_CHECK(!notices.shown(1.0F));
    notices.post({ .text = "undo extrude face", .done = true, .undoable = false });

    const std::optional<Notices::Shown> first = notices.shown(10.0F);
    BOOST_REQUIRE(first.has_value());
    BOOST_CHECK_EQUAL(first.value_or(Notices::Shown()).text, "Undo extrude face");
    BOOST_CHECK_SMALL(first.value_or(Notices::Shown{ .alpha = 1.0F }).alpha, CLOSE);

    BOOST_CHECK_CLOSE(notices.shown(10.0F + (Notices::FADE / 2.0F)).value_or(Notices::Shown()).alpha, 0.5F, 0.1F);
    BOOST_CHECK_CLOSE(notices.shown(11.0F).value_or(Notices::Shown()).alpha, 1.0F, 0.1F);
    BOOST_CHECK_CLOSE(notices.shown(10.0F + Notices::SHOWN - (Notices::FADE / 2.0F)).value_or(Notices::Shown()).alpha, 0.5F, 0.1F);
    BOOST_CHECK(!notices.shown(10.0F + Notices::SHOWN));
    BOOST_CHECK(!notices.shown(10.0F));
}

/**
 * A notice says whether it was done and whether undo takes it back, as it was posted.
 **/
BOOST_AUTO_TEST_CASE(a_notice_keeps_whether_it_was_done_and_can_be_undone) {
    Notices notices;
    notices.post({ .text = "extruded 1 face", .done = true, .undoable = true });
    const Notices::Shown done = notices.shown(1.0F).value_or(Notices::Shown());
    BOOST_CHECK(done.done);
    BOOST_CHECK(done.undoable);

    notices.post({ .text = "nothing to extrude", .done = false, .undoable = false });
    const Notices::Shown refused = notices.shown(2.0F).value_or(Notices::Shown());
    BOOST_CHECK_EQUAL(refused.text, "Nothing to extrude");
    BOOST_CHECK(!refused.done);
    BOOST_CHECK(!refused.undoable);
}

/**
 * A new notice replaces the one showing and starts its time again. An empty one is ignored.
 **/
BOOST_AUTO_TEST_CASE(a_new_notice_replaces_the_one_showing) {
    Notices notices;
    notices.post({ .text = "first", .done = true, .undoable = false });
    BOOST_REQUIRE(notices.shown(0.0F).has_value());
    notices.post({ .text = "second", .done = true, .undoable = false });
    notices.post({ .text = "", .done = true, .undoable = false });
    const std::optional<Notices::Shown> second = notices.shown(2.0F);
    BOOST_REQUIRE(second.has_value());
    BOOST_CHECK_EQUAL(second.value_or(Notices::Shown()).text, "Second");
    BOOST_CHECK(notices.shown(2.0F + Notices::SHOWN - 0.5F).has_value());
}

BOOST_AUTO_TEST_SUITE_END()

};  // namespace
