/**
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 **/

#include <api/type/effect/Emitter.h>
#include <api/type/effect/Particle.h>
#include <api/type/effect/State.h>
#include <api/type/effect/Weather.h>

#include <cmath>
#include <limits>

#include <boost/test/unit_test.hpp>

#include <glm/geometric.hpp>
#include <glm/vec3.hpp>

using v3d::type::effect::Emitter;
using v3d::type::effect::Particle;
using v3d::type::effect::State;
using v3d::type::effect::Weather;

namespace {

const float sixtieth = 1.0f / 60.0f;

/**
 * Rain falling straight down at five a second, living longer than any case runs.
 **/
Emitter rain() {
    Emitter emitter;
    emitter.direction = glm::vec3(0.0f, -1.0f, 0.0f);
    emitter.speedMin = 5.0f;
    emitter.speedMax = 5.0f;
    emitter.lifeMin = 100.0f;
    emitter.lifeMax = 100.0f;
    emitter.cap = 100000;
    return emitter;
}

/**
 * Weather of one particle a second a square unit, already falling as hard as it will.
 **/
Weather shower() {
    Weather weather;
    weather.density = 1.0f;
    weather.intensity = 1.0f;
    weather.target = 1.0f;
    return weather;
}

/**
 * A region four by four on the ground and ten high, its corner at x.
 **/
void run(const Emitter& emitter, Weather* weather, State* state, int steps, float x = 0.0f) {
    for (int step = 0; step < steps; step++) {
        v3d::type::effect::fall(emitter, weather, state, glm::vec3(x, 0.0f, 0.0f),
            glm::vec3(x + 4.0f, 10.0f, 4.0f), sixtieth);
    }
}

};  // namespace

BOOST_AUTO_TEST_SUITE(weather_test)

/**
 * At full intensity, what is falling settles at the rate times the time a drop takes to fall:
 * sixteen square units a second, for the two seconds ten units take at five a second. A drop
 * spawned at the top is gone the step it passes below the ground, which is 121 steps of falling.
 **/
BOOST_AUTO_TEST_CASE(weather_settles_at_its_rate_times_its_fall_test) {
    const Emitter emitter = rain();
    Weather weather = shower();
    State state(1);

    run(emitter, &weather, &state, 600);
    BOOST_CHECK(state.particles.size() >= 31u);
    BOOST_CHECK(state.particles.size() <= 34u);

    bool above = true;
    for (const Particle& particle : state.particles) {
        above = above && particle.position.y >= 0.0f && particle.position.y <= 10.0f;
    }
    BOOST_CHECK(above);
}

/**
 * Turning the weather off eases it to nothing at its rate, rather than stopping it, and what
 * was falling then falls out.
 **/
BOOST_AUTO_TEST_CASE(weather_eases_to_its_target_test) {
    const Emitter emitter = rain();
    Weather weather = shower();
    weather.ease = 0.5f;
    State state(1);

    run(emitter, &weather, &state, 180);
    weather.target = 0.0f;
    run(emitter, &weather, &state, 60);
    BOOST_CHECK_CLOSE(weather.intensity, 0.5f, 1e-3f);
    BOOST_CHECK(!state.particles.empty());

    run(emitter, &weather, &state, 60);
    BOOST_CHECK_SMALL(weather.intensity, 1e-5f);
    // the steps' easing sums to a hair short of the whole way in float, and the next step lands
    // on the target exactly rather than passing it
    run(emitter, &weather, &state, 1);
    BOOST_CHECK_EQUAL(weather.intensity, 0.0f);
    run(emitter, &weather, &state, 130);
    BOOST_CHECK(state.particles.empty());
}

/**
 * A region that moves keeps every particle inside it, bringing one that leaves across a side
 * in at the opposite side with its previous position, so nothing is drawn sweeping across.
 **/
BOOST_AUTO_TEST_CASE(weather_follows_a_moving_region_test) {
    const Emitter emitter = rain();
    Weather weather = shower();
    State state(1);

    run(emitter, &weather, &state, 120);
    bool inside = true;
    bool steady = true;
    for (int step = 0; step < 300; step++) {
        const float x = 0.05f * static_cast<float>(step);
        run(emitter, &weather, &state, 1, x);
        for (const Particle& particle : state.particles) {
            inside = inside && particle.position.x >= x && particle.position.x < x + 4.0f;
            inside = inside && particle.position.z >= 0.0f && particle.position.z < 4.0f;
            steady = steady && glm::length(particle.position - particle.previous) <= 5.0f * sixtieth + 1e-4f;
        }
    }
    BOOST_CHECK(inside);
    BOOST_CHECK(steady);
}

/**
 * The wind is an acceleration, so a drop launched straight down is carried sideways at the
 * wind times its age.
 **/
BOOST_AUTO_TEST_CASE(weather_is_carried_by_the_wind_test) {
    const Emitter emitter = rain();
    Weather weather = shower();
    weather.wind = glm::vec3(2.0f, 0.0f, 0.0f);
    State state(1);

    run(emitter, &weather, &state, 90);
    BOOST_REQUIRE(!state.particles.empty());
    bool carried = true;
    for (const Particle& particle : state.particles) {
        carried = carried && std::abs(particle.velocity.x - (2.0f * particle.age)) < 1e-4f;
    }
    BOOST_CHECK(carried);
}

/**
 * An ease below nothing moves the intensity nowhere, and a target outside 0 to 1 eases only as
 * far as the end of that range.
 **/
BOOST_AUTO_TEST_CASE(weather_out_of_range_settings_are_held_in_range_test) {
    const Emitter emitter = rain();
    Weather weather = shower();
    weather.intensity = 0.5f;
    weather.ease = -1.0f;
    State state(1);
    run(emitter, &weather, &state, 10);
    BOOST_CHECK_EQUAL(weather.intensity, 0.5f);

    weather.ease = 10.0f;
    weather.target = 4.0f;
    run(emitter, &weather, &state, 10);
    BOOST_CHECK_EQUAL(weather.intensity, 1.0f);
}

/**
 * A target that is not a number leaves the intensity where it is, rather than turning it into
 * a NaN that no later target could recover from.
 **/
BOOST_AUTO_TEST_CASE(weather_a_target_that_is_not_a_number_is_ignored_test) {
    const Emitter emitter = rain();
    Weather weather = shower();
    weather.intensity = 0.5f;
    weather.target = std::nanf("");
    State state(1);
    run(emitter, &weather, &state, 10);
    BOOST_CHECK_EQUAL(weather.intensity, 0.5f);
    weather.target = 1.0f;
    run(emitter, &weather, &state, 1000);
    BOOST_CHECK_EQUAL(weather.intensity, 1.0f);
}

BOOST_AUTO_TEST_SUITE_END()
