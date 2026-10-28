/**
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 **/

#include <api/audio/Events.h>
#include <api/audio/Listener.h>
#include <api/audio/NullEvents.h>

#include <boost/test/unit_test.hpp>

#include <glm/vec3.hpp>

namespace {

BOOST_AUTO_TEST_SUITE(events_test)

/**
 * The null backend takes a bank, an event and a parameter, and reports each as not done. A game
 * reads those as silence. A backend that only had to not throw would pass with true returns,
 * which would tell a game that a sound played when none did.
 **/
BOOST_AUTO_TEST_CASE(the_null_backend_plays_nothing_and_says_so) {
    v3d::audio::NullEvents events;
    BOOST_TEST(!events.bank("Master.bank"));
    BOOST_TEST(!events.play("event:/UI/Cancel"));
    BOOST_TEST(!events.parameter("Intensity", 0.5F));

    // an event to be kept starts as no event, and nothing can be stopped or is playing
    BOOST_CHECK(events.start("event:/Ambience/Country") == v3d::audio::Instance::None);
    BOOST_TEST(!events.stop(v3d::audio::Instance::None, v3d::audio::Stop::FadeOut));
    BOOST_TEST(!events.stop(v3d::audio::Instance{1}, v3d::audio::Stop::Immediately));
    BOOST_TEST(!events.playing(v3d::audio::Instance::None));
    BOOST_TEST(!events.playing(v3d::audio::Instance{1}));
    events.update();
    BOOST_TEST(events.name() == "null");
}

/**
 * The null backend has no mixer, so it sets no volume and pauses no bus, and has nothing to
 * read back from either.
 **/
BOOST_AUTO_TEST_CASE(the_null_backend_has_no_buses) {
    v3d::audio::NullEvents events;
    BOOST_TEST(!events.volume("bus:/Music", 0.5F));
    BOOST_TEST(!events.volume("vca:/SFX", 0.5F));
    BOOST_TEST(!events.volume("bus:/Music").has_value());
    BOOST_TEST(!events.paused("bus:/", true));
    BOOST_TEST(!events.paused("bus:/").has_value());
}

/**
 * The null backend places nothing and hears from no listener, so a game that places its events
 * runs silent on it as it does for unplaced ones.
 **/
BOOST_AUTO_TEST_CASE(the_null_backend_places_nothing) {
    v3d::audio::NullEvents events;
    BOOST_TEST(!events.play("event:/UI/Cancel", glm::vec3(1.0F, 0.0F, 0.0F)));
    BOOST_CHECK(events.start("event:/Ambience/Country", glm::vec3(1.0F, 0.0F, 0.0F)) == v3d::audio::Instance::None);
    BOOST_TEST(!events.position(v3d::audio::Instance{1}, glm::vec3(0.0F)));
    BOOST_TEST(!events.position(v3d::audio::Instance{1}).has_value());
    BOOST_TEST(!events.listener(v3d::audio::Listener()));
}

BOOST_AUTO_TEST_SUITE_END()

};  // namespace
