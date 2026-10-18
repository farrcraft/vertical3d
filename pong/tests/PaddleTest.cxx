/**
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 **/

#include <pong/src/Paddle.h>

#include <boost/test/unit_test.hpp>

#include <entt/entt.hpp>
#include <glm/glm.hpp>

namespace {

BOOST_AUTO_TEST_CASE(paddle_components_test) {
    entt::registry registry;
    Paddle paddle(&registry);

    BOOST_TEST(paddle.position() == 0.0F);
    BOOST_TEST(paddle.offset() == 0.0F);
    BOOST_TEST(paddle.score() == 0);
    BOOST_TEST(!paddle.up());
    BOOST_TEST(!paddle.down());
    BOOST_TEST(paddle.size() == 15.0F);
    BOOST_TEST(paddle.length() == 50.0F);
    BOOST_TEST((paddle.color() == glm::vec3(1.0F, 1.0F, 1.0F)));
}

/**
 * position() writes an absolute position, and move() adds to the current one.
 **/
BOOST_AUTO_TEST_CASE(paddle_move_test) {
    entt::registry registry;
    Paddle paddle(&registry);

    paddle.position(100.0F);
    BOOST_TEST(paddle.position() == 100.0F);

    paddle.move(-1.5F);
    BOOST_TEST(paddle.position() == 98.5F);

    paddle.move(3.0F);
    BOOST_TEST(paddle.position() == 101.5F);
}

/**
 * reset() clears the score, and leaves the travel flags and the position alone. The travel
 * flags follow whichever key is held.
 **/
BOOST_AUTO_TEST_CASE(paddle_reset_test) {
    entt::registry registry;
    Paddle paddle(&registry);

    paddle.score(4);
    paddle.up(true);
    paddle.position(300.0F);

    paddle.reset();

    BOOST_TEST(paddle.score() == 0);
    BOOST_TEST(paddle.up());
    BOOST_TEST(paddle.position() == 300.0F);
}

BOOST_AUTO_TEST_CASE(paddle_travel_test) {
    entt::registry registry;
    Paddle paddle(&registry);

    paddle.up(true);
    paddle.down(true);
    BOOST_TEST(paddle.up());
    BOOST_TEST(paddle.down());

    paddle.up(false);
    BOOST_TEST(!paddle.up());
    BOOST_TEST(paddle.down());
}

};  // namespace
