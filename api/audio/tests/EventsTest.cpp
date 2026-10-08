/**
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 **/

#include <api/audio/NullEvents.h>

#include <boost/test/unit_test.hpp>

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
    BOOST_TEST(!events.parameter("Intensity", 0.5f));
    events.update();
    BOOST_TEST(events.name() == "null");
}

BOOST_AUTO_TEST_SUITE_END()
