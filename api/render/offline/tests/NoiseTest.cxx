/**
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 **/

#include <api/render/offline/Noise.h>

#include <cmath>
#include <limits>

#include <boost/test/unit_test.hpp>

#include <glm/vec3.hpp>

/**
 * SL's noise is in [0, 1], and is 0.5 on every lattice point, where improved noise is zero.
 **/
BOOST_AUTO_TEST_CASE(noise_range_test) {
    float lowest = 1.0f;
    float highest = 0.0f;
    for (int i = 0; i < 4000; i++) {
        const float x = static_cast<float>(i) * 0.137f;
        const glm::vec3 at(x, std::sin(x) * 7.3f, std::cos(x * 0.71f) * 5.1f);
        const float value = v3d::render::offline::noise(at);
        lowest = value < lowest ? value : lowest;
        highest = value > highest ? value : highest;
    }
    BOOST_CHECK_GE(lowest, 0.0f);
    BOOST_CHECK_LE(highest, 1.0f);
    // it varies across the range, rather than staying near its middle
    BOOST_CHECK_LT(lowest, 0.3f);
    BOOST_CHECK_GT(highest, 0.7f);

    BOOST_CHECK_EQUAL(v3d::render::offline::noise(glm::vec3(3.0f, -2.0f, 7.0f)), 0.5f);
}

/**
 * Close points are close values: a step a thousandth of a cell across moves the value by
 * little more than a thousandth times the steepest gradient improved noise has.
 **/
BOOST_AUTO_TEST_CASE(noise_continuous_test) {
    for (int i = 0; i < 500; i++) {
        const glm::vec3 at(static_cast<float>(i) * 0.0731f, static_cast<float>(i) * 0.0419f, 1.3f);
        const float here = v3d::render::offline::noise(at);
        const float there = v3d::render::offline::noise(at + glm::vec3(0.001f, 0.0f, 0.0f));
        BOOST_CHECK_SMALL(here - there, 0.005f);
    }
}

/**
 * The same point is the same value on every call, so a reference image can pin a shader
 * that uses it.
 **/
BOOST_AUTO_TEST_CASE(noise_deterministic_test) {
    const glm::vec3 at(1.25f, 2.5f, -3.75f);
    const float first = v3d::render::offline::noise(at);
    BOOST_CHECK_EQUAL(v3d::render::offline::noise(at), first);
    BOOST_CHECK_NE(first, 0.5f);
    // the lattice repeats every 256 cells, as Perlin's does
    BOOST_CHECK_CLOSE(v3d::render::offline::noise(at + glm::vec3(256.0f, 0.0f, 0.0f)), first, 0.01f);
}

/**
 * A point that is not finite reads 0.5, the value of every lattice point. A coordinate past
 * the range of an int has a defined cell. Every float that large is a whole number of
 * periods, so its cell is 0. It reads as the coordinate a whole number of periods nearer the
 * origin does. A plain conversion to int is undefined there, and gives cell 0 under MSVC as
 * well. These checks pin the result, and cannot tell the two apart on that compiler.
 **/
BOOST_AUTO_TEST_CASE(noise_far_and_not_finite_test) {
    const float nan = std::numeric_limits<float>::quiet_NaN();
    const float infinity = std::numeric_limits<float>::infinity();
    BOOST_CHECK_EQUAL(v3d::render::offline::noise(glm::vec3(nan, 1.5f, 2.5f)), 0.5f);
    BOOST_CHECK_EQUAL(v3d::render::offline::noise(glm::vec3(1.5f, infinity, 2.5f)), 0.5f);
    BOOST_CHECK_EQUAL(v3d::render::offline::noise(glm::vec3(1.5f, 2.5f, -infinity)), 0.5f);

    // every float this large is a whole number of periods, so the point is a lattice point
    const float distant = 1099511627776.0f;
    BOOST_CHECK_EQUAL(v3d::render::offline::noise(glm::vec3(distant, -distant, 3.0e38f)), 0.5f);

    // 2^31 + 768 is three periods past 2^31, and reads as 768 does
    const glm::vec3 period(768.0f, 1.25f, 2.75f);
    const glm::vec3 beyond(2147484416.0f, 1.25f, 2.75f);
    BOOST_CHECK_EQUAL(v3d::render::offline::noise(beyond), v3d::render::offline::noise(period));
}
