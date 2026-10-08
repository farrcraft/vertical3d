/**
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 **/

#include <api/audio/Events.h>
#include <api/log/Logger.h>

#include <string>

#include <boost/make_shared.hpp>
#include <boost/test/unit_test.hpp>

BOOST_AUTO_TEST_SUITE(fmod_events_test)

/**
 * Built only with the FMOD backend, and run against the banks the SDK ships for its own examples.
 * events() hands back the FMOD backend. A real event plays once its banks are loaded, and an
 * event or a global parameter that no bank holds is refused rather than reported as played.
 **/
BOOST_AUTO_TEST_CASE(the_fmod_backend_plays_an_event_from_a_bank) {
    const boost::shared_ptr<v3d::log::Logger> logger = boost::make_shared<v3d::log::Logger>();
    const boost::shared_ptr<v3d::audio::Events> events = v3d::audio::events(logger);
    BOOST_REQUIRE(events->name() == "fmod");

    const std::string media = V3D_FMOD_EXAMPLE_BANKS;
    BOOST_TEST(!events->play("event:/UI/Cancel"));
    BOOST_REQUIRE(events->bank(media + "/Master.bank"));
    BOOST_REQUIRE(events->bank(media + "/Master.strings.bank"));
    BOOST_REQUIRE(events->bank(media + "/SFX.bank"));
    BOOST_TEST(events->play("event:/UI/Cancel"));
    events->update();

    BOOST_TEST(!events->bank(media + "/No Such.bank"));
    BOOST_TEST(!events->play("event:/No/Such Event"));
    BOOST_TEST(!events->parameter("No Such Parameter", 1.0f));
}

BOOST_AUTO_TEST_SUITE_END()
