/**
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 **/

#include <api/type/effect/Emitter.h>
#include <api/type/effect/Particle.h>
#include <api/type/effect/State.h>

#include <cmath>
#include <cstddef>
#include <vector>

#include <boost/test/unit_test.hpp>

#include <glm/geometric.hpp>
#include <glm/gtc/constants.hpp>

using v3d::type::effect::Emitter;
using v3d::type::effect::Particle;
using v3d::type::effect::State;

namespace {

const float sixtieth = 1.0f / 60.0f;
glm::quat unturned() {
    return glm::identity<glm::quat>();
}

/**
 * An emitter that spawns at rest where it stands and lets its particles live longer than any
 * case runs.
 **/
Emitter still() {
    Emitter emitter;
    emitter.lifeMin = 100.0f;
    emitter.lifeMax = 100.0f;
    return emitter;
}

/**
 * Run an emitter for a number of steps of a sixtieth of a second from the origin.
 **/
void run(const Emitter& emitter, State* state, int steps) {
    for (int step = 0; step < steps; step++) {
        v3d::type::effect::step(emitter, state, glm::vec3(0.0f), unturned(), sixtieth);
    }
}

bool equal(const std::vector<Particle>& a, const std::vector<Particle>& b) {
    if (a.size() != b.size()) {
        return false;
    }
    for (std::size_t particle = 0; particle < a.size(); particle++) {
        if (a[particle].position != b[particle].position || a[particle].velocity != b[particle].velocity ||
            a[particle].lifetime != b[particle].lifetime || a[particle].phase != b[particle].phase) {
            return false;
        }
    }
    return true;
}

};  // namespace

BOOST_AUTO_TEST_SUITE(effect_test)

/**
 * A rate of five a second, stepped sixty times a second for a second, has spawned five. No
 * single step earns a whole particle, so that takes the owed fraction carried between steps,
 * and five twelfths summed in float falls just short of each whole number it should reach.
 **/
BOOST_AUTO_TEST_CASE(effect_a_rate_spawns_what_it_owes_test) {
    Emitter emitter = still();
    emitter.rate = 5.0f;
    State state(1);

    run(emitter, &state, 11);
    BOOST_CHECK_EQUAL(state.particles.size(), 0u);
    run(emitter, &state, 1);
    BOOST_CHECK_EQUAL(state.particles.size(), 1u);
    run(emitter, &state, 48);
    BOOST_CHECK_EQUAL(state.particles.size(), 5u);
}

/**
 * A burst spawns its count at once, and neither a burst nor a rate takes the emitter past its
 * cap.
 **/
BOOST_AUTO_TEST_CASE(effect_a_burst_and_the_cap_test) {
    Emitter emitter = still();
    emitter.cap = 12;
    State state(1);

    v3d::type::effect::burst(emitter, &state, glm::vec3(0.0f), unturned(), 8);
    BOOST_CHECK_EQUAL(state.particles.size(), 8u);
    v3d::type::effect::burst(emitter, &state, glm::vec3(0.0f), unturned(), 8);
    BOOST_CHECK_EQUAL(state.particles.size(), 12u);

    emitter.rate = 600.0f;
    run(emitter, &state, 10);
    BOOST_CHECK_EQUAL(state.particles.size(), 12u);
}

/**
 * A particle born at rest under gravity falls where semi-implicit Euler puts it: after n steps
 * of h, g h^2 n(n+1)/2, which is half g t^2 and the half step of error the integrator adds.
 **/
BOOST_AUTO_TEST_CASE(effect_a_particle_falls_under_gravity_test) {
    Emitter emitter = still();
    emitter.acceleration = glm::vec3(0.0f, -9.8f, 0.0f);
    State state(1);
    v3d::type::effect::burst(emitter, &state, glm::vec3(0.0f), unturned(), 1);

    const int steps = 30;
    run(emitter, &state, steps);

    const float h = sixtieth;
    const float fallen = -9.8f * h * h * static_cast<float>(steps * (steps + 1)) / 2.0f;
    BOOST_CHECK_CLOSE(state.particles[0].position.y, fallen, 1e-3f);
    BOOST_CHECK_CLOSE(state.particles[0].velocity.y, -9.8f * h * static_cast<float>(steps), 1e-3f);
}

/**
 * Every particle is gone by the end of its life, and none is ever kept older than it.
 **/
BOOST_AUTO_TEST_CASE(effect_a_particle_dies_at_the_end_of_its_life_test) {
    Emitter emitter;
    emitter.rate = 60.0f;
    emitter.lifeMin = 0.25f;
    emitter.lifeMax = 0.5f;
    State state(3);

    bool young = true;
    for (int step = 0; step < 120; step++) {
        run(emitter, &state, 1);
        for (const Particle& particle : state.particles) {
            young = young && particle.age < particle.lifetime;
        }
    }
    BOOST_CHECK(young);
    BOOST_CHECK(!state.particles.empty());

    emitter.rate = 0.0f;
    run(emitter, &state, 31);
    BOOST_CHECK(state.particles.empty());
}

/**
 * Two emitters from one seed make the same particles step for step, and another seed does not.
 **/
BOOST_AUTO_TEST_CASE(effect_a_seed_fixes_the_particles_test) {
    Emitter emitter;
    emitter.rate = 30.0f;
    emitter.shape = Emitter::Shape::Box;
    emitter.extent = glm::vec3(1.0f, 0.5f, 1.0f);
    emitter.spread = 0.5f;
    emitter.speedMin = 1.0f;
    emitter.speedMax = 2.0f;
    emitter.lifeMin = 0.5f;
    emitter.lifeMax = 1.5f;
    State first(42);
    State second(42);
    State other(43);

    bool same = true;
    for (int step = 0; step < 90; step++) {
        run(emitter, &first, 1);
        run(emitter, &second, 1);
        run(emitter, &other, 1);
        same = same && equal(first.particles, second.particles);
    }
    BOOST_CHECK(same);
    BOOST_CHECK(!equal(first.particles, other.particles));
}

/**
 * A particle born this step stands where it was born, with its previous position there too, so
 * a frame drawn before its first move does not sweep it in from anywhere.
 **/
BOOST_AUTO_TEST_CASE(effect_a_newborn_has_no_past_test) {
    Emitter emitter = still();
    emitter.speedMin = 3.0f;
    emitter.speedMax = 3.0f;
    State state(1);

    v3d::type::effect::burst(emitter, &state, glm::vec3(2.0f, 0.0f, 0.0f), unturned(), 1);
    BOOST_CHECK(state.particles[0].position == glm::vec3(2.0f, 0.0f, 0.0f));
    BOOST_CHECK(state.particles[0].previous == state.particles[0].position);

    run(emitter, &state, 1);
    BOOST_CHECK(state.particles[0].previous == glm::vec3(2.0f, 0.0f, 0.0f));
    BOOST_CHECK_CLOSE(state.particles[0].position.y, 3.0f * sixtieth, 1e-3f);

    // and each step after keeps where the one before left it
    const glm::vec3 first = state.particles[0].position;
    run(emitter, &state, 1);
    BOOST_CHECK(state.particles[0].previous == first);
}

/**
 * The spawn shape and the launch direction turn with the emitter: a cone straight up, turned a
 * quarter about z, launches along -x.
 **/
BOOST_AUTO_TEST_CASE(effect_the_emitter_is_turned_test) {
    Emitter emitter = still();
    emitter.speedMin = 1.0f;
    emitter.speedMax = 1.0f;
    State state(1);

    const glm::quat quarter = glm::angleAxis(glm::half_pi<float>(), glm::vec3(0.0f, 0.0f, 1.0f));
    v3d::type::effect::burst(emitter, &state, glm::vec3(0.0f), quarter, 1);
    BOOST_CHECK_SMALL(glm::length(state.particles[0].velocity - glm::vec3(-1.0f, 0.0f, 0.0f)), 1e-6f);
}

/**
 * Particles born in a sphere lie within its radius, and drag slows a moving one.
 **/
BOOST_AUTO_TEST_CASE(effect_a_sphere_and_drag_test) {
    Emitter emitter = still();
    emitter.shape = Emitter::Shape::Sphere;
    emitter.extent = glm::vec3(2.0f);
    emitter.speedMin = 4.0f;
    emitter.speedMax = 4.0f;
    emitter.drag = 1.0f;
    emitter.cap = 1000;
    State state(5);

    v3d::type::effect::burst(emitter, &state, glm::vec3(0.0f), unturned(), 1000);
    bool inside = true;
    for (const Particle& particle : state.particles) {
        inside = inside && glm::length(particle.position) <= 2.0f + 1e-5f;
    }
    BOOST_CHECK(inside);

    run(emitter, &state, 1);
    BOOST_CHECK_CLOSE(glm::length(state.particles[0].velocity), 4.0f * (1.0f - sixtieth), 1e-3f);
}

/**
 * A rate below nothing earns nothing, rather than a count that wraps round to billions when it
 * is made unsigned.
 **/
BOOST_AUTO_TEST_CASE(effect_a_negative_rate_owes_nothing_test) {
    State state(1);
    BOOST_CHECK_EQUAL(owing(&state, -5.0f, 1.0f), 0u);
    BOOST_CHECK_EQUAL(owing(&state, 5.0f, 0.2f), 1u);
}

BOOST_AUTO_TEST_SUITE_END()
