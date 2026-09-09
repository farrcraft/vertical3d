/**
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 **/

#include <pong/src/Paddle.h>

#include <boost/test/unit_test.hpp>

#include <entt/entt.hpp>
#include <glm/glm.hpp>

BOOST_AUTO_TEST_CASE(paddle_components_test) {
    entt::registry registry;
    Paddle paddle(&registry);

    BOOST_TEST(paddle.position() == 0.0f);
    BOOST_TEST(paddle.offset() == 0.0f);
    BOOST_TEST(paddle.score() == 0);
    BOOST_TEST(!paddle.up());
    BOOST_TEST(!paddle.down());
    BOOST_TEST(paddle.size() == 15.0f);
    BOOST_TEST(paddle.length() == 50.0f);
    BOOST_TEST((paddle.color() == glm::vec3(1.0f, 1.0f, 1.0f)));
}

/**
 * position() writes an absolute and move() adds to it - the scene uses the first to centre a
 * paddle after a point and the second nowhere, so this is what keeps them distinct.
 **/
BOOST_AUTO_TEST_CASE(paddle_move_test) {
    entt::registry registry;
    Paddle paddle(&registry);

    paddle.position(100.0f);
    BOOST_TEST(paddle.position() == 100.0f);

    paddle.move(-1.5f);
    BOOST_TEST(paddle.position() == 98.5f);

    paddle.move(3.0f);
    BOOST_TEST(paddle.position() == 101.5f);
}

/**
 * reset() clears the score and deliberately leaves the travel flags, which are held by
 * whichever key is down at the time.
 **/
BOOST_AUTO_TEST_CASE(paddle_reset_test) {
    entt::registry registry;
    Paddle paddle(&registry);

    paddle.score(4);
    paddle.up(true);
    paddle.position(300.0f);

    paddle.reset();

    BOOST_TEST(paddle.score() == 0);
    BOOST_TEST(paddle.up());
    BOOST_TEST(paddle.position() == 300.0f);
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
