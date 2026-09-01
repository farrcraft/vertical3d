/**
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 **/

#include <boost/test/unit_test.hpp>

#include "../realtime/Frame.h"
#include "../realtime/Pass.h"

#include <boost/make_shared.hpp>

BOOST_AUTO_TEST_SUITE(frame_test)

/**
 * A frame starts with no passes at all - the engine adds the one it draws through.
 **/
BOOST_AUTO_TEST_CASE(frame_starts_empty) {
    v3d::render::realtime::Frame frame(boost::make_shared<v3d::render::realtime::Context>());

    BOOST_CHECK_EQUAL(frame.passes().size(), 0);
}

/**
 * Asking for a pass by name twice gives the same pass rather than a second one, which is
 * what lets several parts of an app submit into one pass without co-ordinating.
 **/
BOOST_AUTO_TEST_CASE(passes_are_created_once_and_kept_in_order) {
    v3d::render::realtime::Frame frame(boost::make_shared<v3d::render::realtime::Context>());

    boost::shared_ptr<v3d::render::realtime::Pass> scene = frame.pass("scene");
    boost::shared_ptr<v3d::render::realtime::Pass> overlay = frame.pass("overlay");
    boost::shared_ptr<v3d::render::realtime::Pass> again = frame.pass("scene");

    BOOST_CHECK_EQUAL(frame.passes().size(), 2);
    BOOST_CHECK(scene == again);
    BOOST_CHECK_EQUAL(frame.passes()[0]->name(), "scene");
    BOOST_CHECK_EQUAL(frame.passes()[1]->name(), "overlay");
}

/**
 * A pass clears by default, so a frame whose first pass was never configured still starts
 * from a known image rather than from whatever was last presented.
 **/
BOOST_AUTO_TEST_CASE(a_pass_clears_by_default) {
    v3d::render::realtime::Pass pass("colour");

    BOOST_CHECK(pass.clears());
    BOOST_CHECK(!pass.depth());

    pass.keepColour();
    BOOST_CHECK(!pass.clears());

    pass.clearColour(glm::vec4(1.0f, 0.0f, 0.0f, 1.0f));
    BOOST_CHECK(pass.clears());
    BOOST_CHECK_EQUAL(pass.clearColour().r, 1.0f);
}

/**
 * Items come back in submission order. Sorting is the recorder's job and does not happen
 * yet, so a pass must not reorder anything behind the caller's back.
 **/
BOOST_AUTO_TEST_CASE(items_keep_submission_order) {
    v3d::render::realtime::Pass pass("colour");

    v3d::render::realtime::DrawItem first;
    first.key.layer = 5;
    first.vertices = 6;
    pass.submit(first);

    v3d::render::realtime::DrawItem second;
    second.key.layer = 1;
    second.vertices = 3;
    pass.submit(second);

    BOOST_REQUIRE_EQUAL(pass.items().size(), 2);
    BOOST_CHECK_EQUAL(pass.items()[0].key.layer, 5);
    BOOST_CHECK_EQUAL(pass.items()[1].key.layer, 1);
}

/**
 * Resetting a frame empties the queues but keeps the passes and how they were configured,
 * because the engine builds them once and draws with them every frame after that.
 **/
BOOST_AUTO_TEST_CASE(reset_empties_the_queues_and_keeps_the_passes) {
    v3d::render::realtime::Frame frame(boost::make_shared<v3d::render::realtime::Context>());

    boost::shared_ptr<v3d::render::realtime::Pass> pass = frame.pass("colour");
    pass->clearColour(glm::vec4(0.0f, 1.0f, 0.0f, 1.0f));
    pass->submit(v3d::render::realtime::DrawItem());
    pass->submit(v3d::render::realtime::DrawItem());

    BOOST_CHECK_EQUAL(pass->items().size(), 2);

    frame.reset();

    BOOST_CHECK_EQUAL(frame.passes().size(), 1);
    BOOST_CHECK_EQUAL(pass->items().size(), 0);
    BOOST_CHECK_EQUAL(pass->clearColour().g, 1.0f);
}

/**
 * A draw item defaults to one instance, since every draw is at least that.
 **/
BOOST_AUTO_TEST_CASE(a_draw_item_defaults_to_one_instance) {
    v3d::render::realtime::DrawItem item;

    BOOST_CHECK_EQUAL(item.instances, 1);
    BOOST_CHECK_EQUAL(item.vertices, 0);
    BOOST_CHECK(!item.pipeline.valid());
    BOOST_CHECK(!item.material.valid());
    BOOST_CHECK(!item.record);
}

BOOST_AUTO_TEST_SUITE_END()
