/**
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 **/

#include <pong/src/Ball.h>

#include <boost/test/unit_test.hpp>

#include <entt/entt.hpp>
#include <glm/glm.hpp>

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
 * The direction is a velocity in units per second, so a move scales it by the step - which
 * is why the scene multiplies the direction to speed the ball up rather than scaling
 * anything else.
 **/
BOOST_AUTO_TEST_CASE(ball_move_test) {
    entt::registry registry;
    Ball ball(&registry);

    ball.position(glm::vec2(10.0f, 10.0f));
    ball.direction(glm::vec2(2.0f, -1.0f));

    ball.move(1.0f);
    BOOST_TEST((ball.position() == glm::vec2(12.0f, 9.0f)));

    ball.move(1.0f);
    BOOST_TEST((ball.position() == glm::vec2(14.0f, 8.0f)));
}

/**
 * Half the step is half the distance. Two moves of half a step land where one whole one
 * would, which is what makes the ball's speed a property of the ball rather than of how
 * often the loop got round to it.
 **/
BOOST_AUTO_TEST_CASE(ball_move_scales_by_the_step_test) {
    entt::registry registry;
    Ball whole(&registry);
    Ball halves(&registry);

    whole.position(glm::vec2(0.0f, 0.0f));
    whole.direction(glm::vec2(60.0f, -30.0f));
    halves.position(glm::vec2(0.0f, 0.0f));
    halves.direction(glm::vec2(60.0f, -30.0f));

    whole.move(1.0f / 60.0f);
    halves.move(1.0f / 120.0f);
    halves.move(1.0f / 120.0f);

    BOOST_TEST(whole.position().x == halves.position().x, boost::test_tools::tolerance(0.0001f));
    BOOST_TEST(whole.position().y == halves.position().y, boost::test_tools::tolerance(0.0001f));
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
