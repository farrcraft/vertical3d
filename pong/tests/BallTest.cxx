/**
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 **/

#include <pong/src/Ball.h>

#include <boost/test/unit_test.hpp>

#include <entt/entt.hpp>
#include <glm/glm.hpp>

namespace {

/**
 * A ball is an entity in the app's registry rather than a value, so everything it holds is a
 * component read back through it.
 **/
BOOST_AUTO_TEST_CASE(ball_components_test) {
    entt::registry registry;
    Ball ball(&registry);

    BOOST_TEST((ball.position() == glm::vec2(0.0F, 0.0F)));
    BOOST_TEST((ball.direction() == glm::vec2(0.0F, 0.0F)));
    BOOST_TEST(ball.size() == 1.0F);

    ball.position(glm::vec2(3.0F, 4.0F));
    ball.direction(glm::vec2(-1.0F, 0.5F));
    ball.size(10.0F);

    BOOST_TEST((ball.position() == glm::vec2(3.0F, 4.0F)));
    BOOST_TEST((ball.direction() == glm::vec2(-1.0F, 0.5F)));
    BOOST_TEST(ball.size() == 10.0F);
}

/**
 * The direction is a velocity in units per second, so a move scales it by the step.
 **/
BOOST_AUTO_TEST_CASE(ball_move_test) {
    entt::registry registry;
    Ball ball(&registry);

    ball.position(glm::vec2(10.0F, 10.0F));
    ball.direction(glm::vec2(2.0F, -1.0F));

    ball.move(1.0F);
    BOOST_TEST((ball.position() == glm::vec2(12.0F, 9.0F)));

    ball.move(1.0F);
    BOOST_TEST((ball.position() == glm::vec2(14.0F, 8.0F)));
}

/**
 * Half the step is half the distance. Two moves of half a step land where one whole one
 * would, so the ball's speed does not depend on how often the loop steps it.
 **/
BOOST_AUTO_TEST_CASE(ball_move_scales_by_the_step_test) {
    entt::registry registry;
    Ball whole(&registry);
    Ball halves(&registry);

    whole.position(glm::vec2(0.0F, 0.0F));
    whole.direction(glm::vec2(60.0F, -30.0F));
    halves.position(glm::vec2(0.0F, 0.0F));
    halves.direction(glm::vec2(60.0F, -30.0F));

    whole.move(1.0F / 60.0F);
    halves.move(1.0F / 120.0F);
    halves.move(1.0F / 120.0F);

    BOOST_TEST(whole.position().x == halves.position().x, boost::test_tools::tolerance(0.0001F));
    BOOST_TEST(whole.position().y == halves.position().y, boost::test_tools::tolerance(0.0001F));
}

/**
 * Two balls in one registry are two entities, so neither reads the other's components.
 **/
BOOST_AUTO_TEST_CASE(ball_entities_are_distinct_test) {
    entt::registry registry;
    Ball first(&registry);
    Ball second(&registry);

    first.position(glm::vec2(1.0F, 1.0F));
    second.position(glm::vec2(2.0F, 2.0F));

    BOOST_TEST((first.position() == glm::vec2(1.0F, 1.0F)));
    BOOST_TEST((second.position() == glm::vec2(2.0F, 2.0F)));
}

};  // namespace
