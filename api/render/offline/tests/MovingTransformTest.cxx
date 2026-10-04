/**
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 **/

#include <api/render/offline/MovingTransform.h>

#include <vector>

#include <boost/test/unit_test.hpp>

#include <glm/gtc/matrix_transform.hpp>

namespace {

glm::vec3 apply(const glm::mat4x4 & m, const glm::vec3 & point) {
    return glm::vec3(m * glm::vec4(point, 1.0f));
}

glm::mat4x4 translation(float x) {
    return glm::translate(glm::mat4x4(1.0f), glm::vec3(x, 0.0f, 0.0f));
}

};  // namespace

/**
 * A still transformation answers itself exactly at every time, which is what keeps every
 * picture without a motion block the picture it was.
 **/
BOOST_AUTO_TEST_CASE(moving_transform_still_test) {
    const glm::mat4x4 skew(1.0f, 0.3f, 0.0f, 0.0f, 0.0f, 2.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.5f, 0.0f, 1.0f, 2.0f, 3.0f, 1.0f);
    const v3d::render::offline::MovingTransform still(skew);

    BOOST_CHECK(!still.moving());
    BOOST_CHECK(still.at(0.0f) == skew);
    BOOST_CHECK(still.at(0.37f) == skew);
}

/**
 * Two translations in a block are the two ends, and the motion between them is linear, held
 * at the ends outside the block's times.
 **/
BOOST_AUTO_TEST_CASE(moving_transform_translation_test) {
    v3d::render::offline::MovingTransform moving;
    moving.begin({ 0.0f, 1.0f });
    BOOST_CHECK(moving.inBlock());
    moving.concat(translation(0.0f));
    moving.concat(translation(2.0f));
    moving.end();

    BOOST_REQUIRE(moving.moving());
    BOOST_CHECK_EQUAL(moving.times().x, 0.0f);
    BOOST_CHECK_EQUAL(moving.times().y, 1.0f);
    BOOST_CHECK_SMALL(apply(moving.at(0.0f), glm::vec3(0.0f)).x, 1.0e-6f);
    BOOST_CHECK_CLOSE(apply(moving.at(0.25f), glm::vec3(0.0f)).x, 0.5f, 1.0e-3f);
    BOOST_CHECK_CLOSE(apply(moving.at(1.0f), glm::vec3(0.0f)).x, 2.0f, 1.0e-4f);
    BOOST_CHECK_CLOSE(apply(moving.at(5.0f), glm::vec3(0.0f)).x, 2.0f, 1.0e-4f);
}

/**
 * A rotation is interpolated by a quaternion, so halfway between none and a quarter turn is
 * an eighth of one, and a point keeps its distance from the axis throughout.
 **/
BOOST_AUTO_TEST_CASE(moving_transform_rotation_test) {
    v3d::render::offline::MovingTransform moving;
    moving.begin({ 0.0f, 1.0f });
    moving.concat(glm::rotate(glm::mat4x4(1.0f), 0.0f, glm::vec3(0.0f, 0.0f, 1.0f)));
    moving.concat(glm::rotate(glm::mat4x4(1.0f), glm::radians(90.0f), glm::vec3(0.0f, 0.0f, 1.0f)));
    moving.end();

    const glm::vec3 half = apply(moving.at(0.5f), glm::vec3(1.0f, 0.0f, 0.0f));
    BOOST_CHECK_CLOSE(half.x, 0.70710678f, 1.0e-3f);
    BOOST_CHECK_CLOSE(half.y, 0.70710678f, 1.0e-3f);
    const glm::vec3 quarter = apply(moving.at(0.25f), glm::vec3(1.0f, 0.0f, 0.0f));
    BOOST_CHECK_CLOSE(glm::length(quarter), 1.0f, 1.0e-3f);
}

/**
 * A request after the block applies to both ends, so what is placed under a moving
 * transformation moves with it; one before it is the base both ends start from.
 **/
BOOST_AUTO_TEST_CASE(moving_transform_requests_around_a_block_test) {
    v3d::render::offline::MovingTransform moving;
    moving.concat(glm::scale(glm::mat4x4(1.0f), glm::vec3(2.0f)));
    moving.begin({ 0.0f, 1.0f });
    moving.concat(translation(0.0f));
    moving.concat(translation(1.0f));
    moving.end();
    moving.concat(translation(1.0f));

    // scale by two, then the moving translation, then one more unit, all applied to the origin
    BOOST_CHECK_CLOSE(apply(moving.open(), glm::vec3(0.0f)).x, 2.0f, 1.0e-4f);
    BOOST_CHECK_CLOSE(apply(moving.close(), glm::vec3(0.0f)).x, 4.0f, 1.0e-4f);

    const v3d::render::offline::MovingTransform camera = moving.before(translation(-10.0f));
    BOOST_CHECK_CLOSE(apply(camera.close(), glm::vec3(0.0f)).x, -6.0f, 1.0e-4f);

    // and replacing it outside a block stops it
    moving.replace(glm::mat4x4(1.0f));
    BOOST_CHECK(!moving.moving());
}
