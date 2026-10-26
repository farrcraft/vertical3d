/**
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 **/

#include <api/animation/Curve.h>

#include <cmath>
#include <limits>
#include <stdexcept>
#include <vector>

#include <boost/test/unit_test.hpp>

using v3d::animation::Curve;

namespace {

BOOST_AUTO_TEST_SUITE(curve_test)

/**
 * A curve with no keys is 0 everywhere, and one key holds its value everywhere.
 **/
BOOST_AUTO_TEST_CASE(curve_empty_and_single_test) {
    Curve curve;
    BOOST_CHECK(curve.empty());
    BOOST_CHECK_EQUAL(curve.sample(10.0F), 0.0F);

    curve.set(5.0F, 3.0F);
    BOOST_CHECK(!curve.empty());
    BOOST_CHECK_EQUAL(curve.sample(-100.0F), 3.0F);
    BOOST_CHECK_EQUAL(curve.sample(5.0F), 3.0F);
    BOOST_CHECK_EQUAL(curve.sample(100.0F), 3.0F);
}

/**
 * Each interpolation leaves its key as it says, and the end keys hold outside the curve.
 **/
BOOST_AUTO_TEST_CASE(curve_interpolation_test) {
    const std::vector<Curve::Key> keys{
        { .frame = 0.0F, .value = 0.0F, .interpolation = Curve::Interpolation::Linear },
        { .frame = 10.0F, .value = 10.0F, .interpolation = Curve::Interpolation::Step },
        { .frame = 20.0F, .value = 0.0F, .interpolation = Curve::Interpolation::Bezier },
        { .frame = 30.0F, .value = 10.0F } };
    const Curve curve(keys);

    BOOST_CHECK_CLOSE(curve.sample(2.5F), 2.5F, 0.001F);
    BOOST_CHECK_EQUAL(curve.sample(15.0F), 10.0F);
    BOOST_CHECK_EQUAL(curve.sample(19.9F), 10.0F);
    BOOST_CHECK_EQUAL(curve.sample(20.0F), 0.0F);
    BOOST_CHECK_EQUAL(curve.sample(-5.0F), 0.0F);
    BOOST_CHECK_EQUAL(curve.sample(35.0F), 10.0F);

    // the end keys are flat, so a cubic between them eases out and in, and is half way at
    // its middle
    BOOST_CHECK_CLOSE(curve.sample(25.0F), 5.0F, 0.001F);
    BOOST_CHECK_LT(curve.sample(21.0F), 1.0F);
    BOOST_CHECK_GT(curve.sample(29.0F), 9.0F);

    // a frame that is not finite reads as the first key
    BOOST_CHECK_EQUAL(curve.sample(std::numeric_limits<float>::quiet_NaN()), 0.0F);
}

/**
 * An automatic key between two others takes the slope through them, and a peak stays flat.
 **/
BOOST_AUTO_TEST_CASE(curve_automatic_slopes_test) {
    Curve rising;
    rising.set(0.0F, 0.0F);
    rising.set(20.0F, 10.0F);
    rising.set(10.0F, 4.0F);
    BOOST_REQUIRE_EQUAL(rising.keys().size(), 3U);
    BOOST_CHECK_CLOSE(rising.keys()[1].in, 0.5F, 0.001F);
    BOOST_CHECK_CLOSE(rising.keys()[1].out, 0.5F, 0.001F);
    BOOST_CHECK_EQUAL(rising.keys()[0].out, 0.0F);
    BOOST_CHECK_EQUAL(rising.keys()[2].in, 0.0F);

    // a key above both neighbours is a peak, and the curve never rises past it
    Curve peak;
    peak.set(0.0F, 0.0F);
    peak.set(10.0F, 5.0F);
    peak.set(20.0F, 0.0F);
    BOOST_CHECK_EQUAL(peak.keys()[1].out, 0.0F);
    for (int half = 0; half <= 40; half++) {
        BOOST_CHECK_LE(peak.sample(static_cast<float>(half) / 2.0F), 5.0F + 0.0001F);
    }

    // a key whose slopes were given keeps them when the keys around it change
    const std::vector<Curve::Key> keys{ { .frame = 0.0F, .value = 0.0F, .in = 2.0F, .out = 2.0F, .automatic = false },
        { .frame = 10.0F, .value = 1.0F } };
    Curve fixed(keys);
    fixed.set(5.0F, 3.0F);
    BOOST_CHECK_EQUAL(fixed.keys()[0].out, 2.0F);
}

/**
 * A key set on a frame that has one replaces its value and keeps its kind, and removing a key
 * takes only that one.
 **/
BOOST_AUTO_TEST_CASE(curve_set_and_remove_test) {
    Curve curve;
    curve.set(10.0F, 1.0F);
    curve.set(0.0F, 0.0F);
    curve.set(20.0F, 2.0F);
    BOOST_REQUIRE_EQUAL(curve.keys().size(), 3U);
    BOOST_CHECK_EQUAL(curve.keys()[0].frame, 0.0F);
    BOOST_CHECK_EQUAL(curve.keys()[2].frame, 20.0F);

    std::vector<Curve::Key> keys = curve.keys();
    keys[1].interpolation = Curve::Interpolation::Step;
    curve = Curve(keys);
    curve.set(10.0F + (Curve::SAME_FRAME / 2.0F), 7.0F);
    BOOST_REQUIRE_EQUAL(curve.keys().size(), 3U);
    BOOST_CHECK_EQUAL(curve.keys()[1].value, 7.0F);
    BOOST_CHECK(curve.keys()[1].interpolation == Curve::Interpolation::Step);

    BOOST_CHECK(curve.remove(10.0F));
    BOOST_CHECK(!curve.remove(10.0F));
    BOOST_CHECK_EQUAL(curve.keys().size(), 2U);
    BOOST_CHECK_CLOSE(curve.sample(10.0F), 1.0F, 0.001F);
}

/**
 * Keys out of order, on one frame, or not finite make no curve.
 **/
BOOST_AUTO_TEST_CASE(curve_rejects_bad_keys_test) {
    BOOST_CHECK_THROW(Curve({ { .frame = 2.0F }, { .frame = 1.0F } }), std::invalid_argument);
    BOOST_CHECK_THROW(Curve({ { .frame = 1.0F }, { .frame = 1.0F } }), std::invalid_argument);
    BOOST_CHECK_THROW(Curve({ { .frame = 1.0F, .value = std::numeric_limits<float>::infinity() } }),
        std::invalid_argument);
    BOOST_CHECK_NO_THROW(Curve(std::vector<Curve::Key>{}));
}

BOOST_AUTO_TEST_SUITE_END()

};  // namespace
