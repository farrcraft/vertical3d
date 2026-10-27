/**
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 **/

#include <api/brep/BRep.h>
#include <api/log/Logger.h>
#include <vertical3d/src/control/Playback.h>
#include <vertical3d/src/document/Document.h>
#include <vertical3d/src/scene/Item.h>
#include <vertical3d/src/scene/Mesh.h>
#include <vertical3d/src/scene/Scene.h>
#include <vertical3d/src/scene/Timeline.h>

#include <boost/filesystem/operations.hpp>
#include <boost/make_shared.hpp>
#include <boost/shared_ptr.hpp>
#include <boost/test/unit_test.hpp>

namespace {

/**
 * A document whose timeline runs frames 1 to 4 at 10 frames a second. It is never saved, so
 * its project directory is never written.
 **/
boost::shared_ptr<v3d::editor::Document> shortTimeline() {
    const boost::shared_ptr<v3d::editor::Document> document = boost::make_shared<v3d::editor::Document>(
        boost::make_shared<v3d::log::Logger>(), boost::filesystem::temp_directory_path() / "v3d_playback_unwritten");
    v3d::editor::Timeline timeline;
    timeline.rate = 10.0F;
    timeline.start = 1;
    timeline.end = 4;
    timeline.frame = 1;
    document->scene()->timeline(timeline);
    return document;
}

BOOST_AUTO_TEST_SUITE(playback_test)

/**
 * Playing advances a frame for each frame's length of simulated time, keeps what is left of a
 * step for the next one, and starts again at the first frame after the last. A timeline that is
 * not playing does not move.
 **/
BOOST_AUTO_TEST_CASE(playing_advances_at_the_rate_and_wraps) {
    const boost::shared_ptr<v3d::editor::Document> document = shortTimeline();
    v3d::editor::Playback playback(document);

    playback.simulate(0.5F);
    BOOST_CHECK_EQUAL(document->scene()->timeline().frame, 1);

    playback.play();
    playback.simulate(0.05F);
    BOOST_CHECK_EQUAL(document->scene()->timeline().frame, 1);
    playback.simulate(0.06F);
    BOOST_CHECK_EQUAL(document->scene()->timeline().frame, 2);
    playback.simulate(0.3F);
    // three frames on from 2 is past 4, so it comes round to 1
    BOOST_CHECK_EQUAL(document->scene()->timeline().frame, 1);

    playback.stop();
    playback.simulate(1.0F);
    BOOST_CHECK_EQUAL(document->scene()->timeline().frame, 1);
}

/**
 * Stepping moves a frame at a time within the range, and to either end of it.
 **/
BOOST_AUTO_TEST_CASE(stepping_stays_in_the_range) {
    const boost::shared_ptr<v3d::editor::Document> document = shortTimeline();
    v3d::editor::Playback playback(document);

    playback.step(-1);
    BOOST_CHECK_EQUAL(document->scene()->timeline().frame, 1);
    playback.last();
    BOOST_CHECK_EQUAL(document->scene()->timeline().frame, 4);
    playback.step(1);
    BOOST_CHECK_EQUAL(document->scene()->timeline().frame, 4);
    playback.first();
    BOOST_CHECK_EQUAL(document->scene()->timeline().frame, 1);
}

/**
 * Stepping by keys goes to the keyed frames of the items given, or of every item when none is.
 **/
BOOST_AUTO_TEST_CASE(stepping_by_keys_goes_to_keyed_frames) {
    const boost::shared_ptr<v3d::editor::Document> document = shortTimeline();
    const boost::shared_ptr<v3d::editor::Mesh> mesh =
        boost::make_shared<v3d::editor::Mesh>(boost::make_shared<v3d::brep::BRep>());
    document->create(mesh, "mesh");
    document->key({ mesh }, 2);
    document->key({ mesh }, 4);
    v3d::editor::Playback playback(document);

    playback.nextKey({}, true);
    BOOST_CHECK_EQUAL(document->scene()->timeline().frame, 2);
    playback.nextKey({ mesh }, true);
    BOOST_CHECK_EQUAL(document->scene()->timeline().frame, 4);
    playback.nextKey({ mesh }, true);
    BOOST_CHECK_EQUAL(document->scene()->timeline().frame, 4);
    playback.firstKey({});
    BOOST_CHECK_EQUAL(document->scene()->timeline().frame, 2);
    playback.lastKey({});
    BOOST_CHECK_EQUAL(document->scene()->timeline().frame, 4);
    playback.nextKey({}, false);
    BOOST_CHECK_EQUAL(document->scene()->timeline().frame, 2);
}

BOOST_AUTO_TEST_SUITE_END()

};  // namespace
