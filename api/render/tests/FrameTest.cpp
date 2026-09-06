/**
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 **/

#include <vector>

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
 * Items come back in submission order. A pass sorts only when it has been asked to, so it
 * must not reorder anything behind the caller's back.
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

/**
 * A pass draws through the identity until an app gives it a camera. That is what a 2D pass
 * wants - a canvas carries its own projection in a push constant and reads nothing at set 0.
 **/
BOOST_AUTO_TEST_CASE(a_pass_has_an_identity_camera_until_it_is_given_one) {
    v3d::render::realtime::Pass pass("scene");

    BOOST_CHECK_EQUAL(pass.view()[3][3], 1.0f);
    BOOST_CHECK_EQUAL(pass.view()[3][0], 0.0f);
    BOOST_CHECK_EQUAL(pass.projection()[3][0], 0.0f);

    glm::mat4 view(1.0f);
    view[3][0] = 4.0f;
    glm::mat4 projection(1.0f);
    projection[3][1] = 7.0f;
    pass.camera(view, projection);

    BOOST_CHECK_EQUAL(pass.view()[3][0], 4.0f);
    BOOST_CHECK_EQUAL(pass.projection()[3][1], 7.0f);
}

/**
 * A pass records in submission order unless it is told otherwise, because 2D content is
 * painter ordered and the key groups by pipeline and material within a layer - sorting a
 * canvas's batches would put a panel over the text drawn on it.
 **/
BOOST_AUTO_TEST_CASE(a_pass_records_in_submission_order_by_default) {
    v3d::render::realtime::Pass pass("overlay");

    v3d::render::realtime::DrawItem panel;
    panel.key.material = 9;
    panel.vertices = 6;
    pass.submit(panel);

    v3d::render::realtime::DrawItem text;
    text.key.material = 2;
    text.vertices = 12;
    pass.submit(text);

    BOOST_CHECK(!pass.sorts());

    std::vector<const v3d::render::realtime::DrawItem*> ordered;
    pass.ordered(&ordered);

    BOOST_REQUIRE_EQUAL(ordered.size(), 2);
    BOOST_CHECK_EQUAL(ordered[0]->key.material, 9);
    BOOST_CHECK_EQUAL(ordered[1]->key.material, 2);
}

/**
 * A sorted pass hands the recorder its items grouped by the key - layer first, then pipeline,
 * then material - which is what lets the recorder skip rebinding between adjacent items.
 **/
BOOST_AUTO_TEST_CASE(a_sorted_pass_records_in_key_order) {
    v3d::render::realtime::Pass pass("scene");
    pass.sort(true);

    v3d::render::realtime::DrawItem overlay;
    overlay.key.layer = 1;
    overlay.key.pipeline = 0;
    pass.submit(overlay);

    v3d::render::realtime::DrawItem second;
    second.key.layer = 0;
    second.key.pipeline = 3;
    second.key.material = 1;
    pass.submit(second);

    v3d::render::realtime::DrawItem first;
    first.key.layer = 0;
    first.key.pipeline = 3;
    first.key.material = 0;
    pass.submit(first);

    BOOST_CHECK(pass.sorts());

    std::vector<const v3d::render::realtime::DrawItem*> ordered;
    pass.ordered(&ordered);

    BOOST_REQUIRE_EQUAL(ordered.size(), 3);
    BOOST_CHECK_EQUAL(ordered[0]->key.material, 0);
    BOOST_CHECK_EQUAL(ordered[1]->key.material, 1);
    BOOST_CHECK_EQUAL(ordered[2]->key.layer, 1);
}

/**
 * Equal keys keep the order they were submitted in. A run of quads sharing a pipeline and a
 * material depends on it - they differ only in where they are on the screen, and the later
 * one has to stay on top.
 **/
BOOST_AUTO_TEST_CASE(sorting_a_pass_is_stable) {
    v3d::render::realtime::Pass pass("scene");
    pass.sort(true);

    for (uint32_t index = 0; index < 4; index++) {
        v3d::render::realtime::DrawItem item;
        item.firstIndex = index;
        pass.submit(item);
    }

    std::vector<const v3d::render::realtime::DrawItem*> ordered;
    pass.ordered(&ordered);

    BOOST_REQUIRE_EQUAL(ordered.size(), 4);
    for (uint32_t index = 0; index < 4; index++) {
        BOOST_CHECK_EQUAL(ordered[index]->firstIndex, index);
    }
}

/**
 * Sorting is per pass, so an app can hold a sorted scene pass and an unsorted ui pass in one
 * frame - which is what a game drawing an overlay over a 3D world is.
 **/
BOOST_AUTO_TEST_CASE(sorting_is_configured_per_pass) {
    v3d::render::realtime::Frame frame(boost::make_shared<v3d::render::realtime::Context>());

    boost::shared_ptr<v3d::render::realtime::Pass> scene = frame.pass("scene");
    scene->depth(true);
    scene->sort(true);

    boost::shared_ptr<v3d::render::realtime::Pass> overlay = frame.pass("overlay");

    BOOST_CHECK(scene->depth());
    BOOST_CHECK(scene->sorts());
    BOOST_CHECK(!overlay->depth());
    BOOST_CHECK(!overlay->sorts());
}

BOOST_AUTO_TEST_SUITE_END()
