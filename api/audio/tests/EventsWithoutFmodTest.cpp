/**
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 **/

// the case is left out of a build with FMOD, and its includes with it
#ifndef V3D_AUDIO_FMOD
#include <api/audio/Events.h>
#include <api/log/Logger.h>

#include <boost/make_shared.hpp>
#endif

#include <boost/test/unit_test.hpp>

namespace {

BOOST_AUTO_TEST_SUITE(events_without_fmod_test)

#ifndef V3D_AUDIO_FMOD
/**
 * Built only without the FMOD backend, as CI builds. events() hands back the null backend rather
 * than null or a throw, so a game that asks for events always has an object to call.
 *
 * Covers the base: d0a85201 added events(). Checked by having it return null without FMOD,
 * which fails this case.
 **/
BOOST_AUTO_TEST_CASE(events_hands_back_the_null_backend) {
    const boost::shared_ptr<v3d::log::Logger> logger = boost::make_shared<v3d::log::Logger>();
    const boost::shared_ptr<v3d::audio::Events> events = v3d::audio::events(logger);
    BOOST_REQUIRE(events != nullptr);
    BOOST_TEST(events->name() == "null");
    BOOST_TEST(!events->play("event:/UI/Cancel"));
}
#endif

BOOST_AUTO_TEST_SUITE_END()

};  // namespace
