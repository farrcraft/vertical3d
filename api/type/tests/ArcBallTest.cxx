/**
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 **/

#include <api/type/ArcBall.h>

#include <boost/test/unit_test.hpp>

namespace {
// bounds() maps the viewport onto [-1, 1], so the centre of a 640x480 viewport is the
// point that maps to the top of the sphere.
const float kWidth = 640.0f;
const float kHeight = 480.0f;
const float kCentreX = (kWidth - 1.0f) * 0.5f;
const float kCentreY = (kHeight - 1.0f) * 0.5f;
};  // namespace

BOOST_AUTO_TEST_CASE(arcball_test) {
    v3d::type::ArcBall ball;
    ball.bounds(kWidth, kHeight);

    // the centre of the viewport maps to the pole of the sphere
    glm::vec3 centre = ball.map(glm::vec2(kCentreX, kCentreY));
    BOOST_CHECK_CLOSE(centre[0], 0.0f, 0.01f);
    BOOST_CHECK_CLOSE(centre[1], 0.0f, 0.01f);
    BOOST_CHECK_CLOSE(centre[2], 1.0f, 0.01f);

    // a corner falls outside the sphere, so it is normalized onto the equator
    glm::vec3 corner = ball.map(glm::vec2(0.0f, 0.0f));
    BOOST_CHECK_CLOSE(corner[0], -0.70710678f, 0.01f);
    BOOST_CHECK_CLOSE(corner[1], 0.70710678f, 0.01f);
    BOOST_CHECK_EQUAL(corner[2], 0.0f);
    BOOST_CHECK_CLOSE(glm::length(corner), 1.0f, 0.01f);

    // dragging to where the click started is no rotation at all
    ball.click(glm::vec2(kCentreX, kCentreY));
    glm::quat still = ball.drag(glm::vec2(kCentreX, kCentreY));
    BOOST_CHECK_EQUAL(still[0], 0.0f);
    BOOST_CHECK_EQUAL(still[1], 0.0f);
    BOOST_CHECK_EQUAL(still[2], 0.0f);
    BOOST_CHECK_EQUAL(still[3], 0.0f);

    // dragging left of the click rotates about the y axis - the two mapped points and the
    // axis they turn about all lie in the xz plane
    ball.click(glm::vec2(kCentreX, kCentreY));
    glm::quat rotation = ball.drag(glm::vec2(0.0f, kCentreY));
    BOOST_CHECK_EQUAL(rotation[0], 0.0f);
    BOOST_CHECK(rotation[1] != 0.0f);
    BOOST_CHECK_EQUAL(rotation[2], 0.0f);

    // drag leaves the end point as the new start, so an immediate repeat is a no-op
    glm::quat repeated = ball.drag(glm::vec2(0.0f, kCentreY));
    BOOST_CHECK_EQUAL(repeated[0], 0.0f);
    BOOST_CHECK_EQUAL(repeated[1], 0.0f);
    BOOST_CHECK_EQUAL(repeated[2], 0.0f);
    BOOST_CHECK_EQUAL(repeated[3], 0.0f);

    // bounds clamps a degenerate viewport rather than dividing by zero
    v3d::type::ArcBall degenerate;
    degenerate.bounds(0.0f, 0.0f);
    glm::vec3 mapped = degenerate.map(glm::vec2(0.0f, 0.0f));
    BOOST_CHECK(std::isfinite(mapped[0]));
    BOOST_CHECK(std::isfinite(mapped[1]));
    BOOST_CHECK(std::isfinite(mapped[2]));
}
