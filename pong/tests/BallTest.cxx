/**
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 **/

#include <boost/test/unit_test.hpp>

#include <entt/entt.hpp>
#include <glm/glm.hpp>

#include "../src/Ball.h"

/**
 * A ball is an entity in the app's registry rather than a value, so everything it holds is a
 * component read back through it.
 **/
BOOST_AUTO_TEST_CASE(ball_components_test) {
    entt::registry registry;
    Ball ball(&registry);

    BOOST_TEST((ball.position() == glm::vec2(0.0f, 0.0f)));
    BOOST_TEST((ball.direction() == glm::vec2(0.0f, 0.0f)));
    BOOST_TEST(ball.size() == 1.0f);

    ball.position(glm::vec2(3.0f, 4.0f));
    ball.direction(glm::vec2(-1.0f, 0.5f));
    ball.size(10.0f);

    BOOST_TEST((ball.position() == glm::vec2(3.0f, 4.0f)));
    BOOST_TEST((ball.direction() == glm::vec2(-1.0f, 0.5f)));
    BOOST_TEST(ball.size() == 10.0f);
}

/**
 * A move is one step of the direction, so direction carries the speed as well as the heading -
 * which is why the scene multiplies it to speed the ball up rather than scaling anything else.
 **/
BOOST_AUTO_TEST_CASE(ball_move_test) {
    entt::registry registry;
    Ball ball(&registry);

    ball.position(glm::vec2(10.0f, 10.0f));
    ball.direction(glm::vec2(2.0f, -1.0f));

    ball.move();
    BOOST_TEST((ball.position() == glm::vec2(12.0f, 9.0f)));

    ball.move();
    BOOST_TEST((ball.position() == glm::vec2(14.0f, 8.0f)));
}

/**
 * Two balls in one registry are two entities, so neither reads the other's components.
 **/
BOOST_AUTO_TEST_CASE(ball_entities_are_distinct_test) {
    entt::registry registry;
    Ball first(&registry);
    Ball second(&registry);

    first.position(glm::vec2(1.0f, 1.0f));
    second.position(glm::vec2(2.0f, 2.0f));

    BOOST_TEST((first.position() == glm::vec2(1.0f, 1.0f)));
    BOOST_TEST((second.position() == glm::vec2(2.0f, 2.0f)));
}
