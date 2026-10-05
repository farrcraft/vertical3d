/**
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 **/

#include <api/type/Random.h>

#include <algorithm>
#include <array>
#include <cmath>
#include <cstdint>
#include <stdexcept>
#include <vector>

#include <boost/test/unit_test.hpp>

#include <glm/geometric.hpp>
#include <glm/gtc/constants.hpp>
#include <glm/vector_relational.hpp>

using v3d::type::Random;

namespace {

std::vector<uint64_t> draw(Random* random, int count) {
    std::vector<uint64_t> values;
    values.reserve(static_cast<std::size_t>(count));
    for (int value = 0; value < count; value++) {
        values.push_back(random->next());
    }
    return values;
}

};  // namespace

BOOST_AUTO_TEST_SUITE(random_test)

/**
 * The first values from seed 1234567 are splitmix64's published test vector, which pins the
 * generator: a change to it fails here rather than as a particle somewhere new.
 **/
BOOST_AUTO_TEST_CASE(random_is_splitmix64_test) {
    Random random(1234567);

    const std::vector<uint64_t> expected = {
        6457827717110365317ULL, 3203168211198807973ULL, 9817491932198370423ULL,
        4593380528125082431ULL, 16408922859458223821ULL
    };
    BOOST_CHECK(draw(&random, 5) == expected);
}

/**
 * Zero is a seed like any other, since a seed can come from a command line or a bug report.
 **/
BOOST_AUTO_TEST_CASE(random_zero_is_a_seed_test) {
    Random random(0);

    const std::vector<uint64_t> values = draw(&random, 2);
    BOOST_CHECK(values[0] != 0);
    BOOST_CHECK(values[0] != values[1]);
}

/**
 * The state is a seed that resumes the sequence rather than restarting it, so a save stores
 * it.
 **/
BOOST_AUTO_TEST_CASE(random_a_restored_state_continues_test) {
    Random original(99);
    draw(&original, 10);

    Random restored(original.state());
    BOOST_CHECK(draw(&original, 16) == draw(&restored, 16));
}

/**
 * below() stays inside its bound, reaches every value in it, and refuses an empty range.
 **/
BOOST_AUTO_TEST_CASE(random_below_test) {
    Random random(2024);

    std::array<int, 20> hits{};
    for (int draw = 0; draw < 20000; draw++) {
        const uint32_t value = random.below(static_cast<uint32_t>(hits.size()));
        BOOST_REQUIRE(value < hits.size());
        hits[value]++;
    }
    for (const int count : hits) {
        BOOST_CHECK(count > 0);
    }
    BOOST_CHECK_EQUAL(random.below(1), 0u);
    BOOST_CHECK_THROW(random.below(0), std::invalid_argument);
}

/**
 * Every float lies in its range over a million draws, and the unit range reaches both its
 * halves.
 **/
BOOST_AUTO_TEST_CASE(random_floats_are_in_range_test) {
    Random random(7);

    float lowest = 1.0f;
    float highest = 0.0f;
    bool inside = true;
    for (int draw = 0; draw < 1000000; draw++) {
        const float unit = random.unit();
        lowest = std::min(lowest, unit);
        highest = std::max(highest, unit);
        const float ranged = random.range(-2.0f, 3.0f);
        inside = inside && unit >= 0.0f && unit < 1.0f && ranged >= -2.0f && ranged < 3.0f;
    }
    BOOST_CHECK(inside);
    BOOST_CHECK(lowest < 0.001f);
    BOOST_CHECK(highest > 0.999f);
}

/**
 * A point in a box lies within both corners on every axis.
 **/
BOOST_AUTO_TEST_CASE(random_a_point_in_a_box_test) {
    Random random(11);

    const glm::vec3 minimum(-1.0f, 2.0f, 10.0f);
    const glm::vec3 maximum(1.0f, 4.0f, 10.5f);
    bool inside = true;
    for (int draw = 0; draw < 10000; draw++) {
        const glm::vec3 point = random.inside(minimum, maximum);
        inside = inside && glm::all(glm::greaterThanEqual(point, minimum)) && glm::all(glm::lessThan(point, maximum));
    }
    BOOST_CHECK(inside);
}

/**
 * Every direction drawn from a cone is a unit vector within its angle of the axis, whichever way
 * the axis points, and a cone of no angle gives the axis.
 **/
BOOST_AUTO_TEST_CASE(random_a_direction_in_a_cone_test) {
    Random random(13);

    const std::array<glm::vec3, 3> axes = {
        glm::vec3(0.0f, 1.0f, 0.0f), glm::vec3(1.0f, 0.0f, 0.0f), glm::vec3(1.0f, -2.0f, 0.5f)
    };
    const float angle = glm::quarter_pi<float>();
    for (const glm::vec3& axis : axes) {
        float widest = 1.0f;
        bool unit = true;
        for (int draw = 0; draw < 10000; draw++) {
            const glm::vec3 direction = random.cone(axis, angle);
            unit = unit && std::abs(glm::length(direction) - 1.0f) < 1e-5f;
            widest = std::min(widest, glm::dot(direction, glm::normalize(axis)));
        }
        BOOST_CHECK(unit);
        BOOST_CHECK(widest >= std::cos(angle) - 1e-5f);
        // and the draws reach out towards the rim rather than bunching at the pole
        BOOST_CHECK(widest < std::cos(angle) + 0.01f);
    }

    const glm::vec3 straight = random.cone(glm::vec3(0.0f, 0.0f, 2.0f), 0.0f);
    BOOST_CHECK_SMALL(glm::length(straight - glm::vec3(0.0f, 0.0f, 1.0f)), 1e-6f);
}

BOOST_AUTO_TEST_SUITE_END()
