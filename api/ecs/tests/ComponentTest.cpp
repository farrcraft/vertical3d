/**
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 **/

#include <api/ecs/component/Color3.h>
#include <api/ecs/component/Position1D.h>
#include <api/ecs/component/Position2D.h>
#include <api/ecs/component/PositionFixed2D.h>

#include <utility>

#include <boost/test/unit_test.hpp>

#include <entt/entt.hpp>

/**
 * Each channel reads back the argument it was named for. The three values are distinct
 * because a uniform colour cannot see two channels exchanged.
 **/
BOOST_AUTO_TEST_CASE(color3_channel_order_test) {
    const v3d::ecs::component::Color3 color(0.25f, 0.5f, 0.75f);

    BOOST_TEST(color.red() == 0.25f);
    BOOST_TEST(color.green() == 0.5f);
    BOOST_TEST(color.blue() == 0.75f);
    BOOST_TEST(color.value().x == 0.25f);
    BOOST_TEST(color.value().y == 0.5f);
    BOOST_TEST(color.value().z == 0.75f);
}

BOOST_AUTO_TEST_CASE(color3_set_test) {
    v3d::ecs::component::Color3 color(0.0f, 0.0f, 0.0f);
    color.set(glm::vec3(0.1f, 0.2f, 0.3f));

    BOOST_TEST(color.red() == 0.1f);
    BOOST_TEST(color.green() == 0.2f);
    BOOST_TEST(color.blue() == 0.3f);
}

/**
 * A component is moved rather than copied - entt relocates one when a pool grows - and every
 * one of these declares the move pair itself instead of taking the implicit one.
 **/
BOOST_AUTO_TEST_CASE(color3_move_test) {
    v3d::ecs::component::Color3 source(0.25f, 0.5f, 0.75f);
    const v3d::ecs::component::Color3 moved(std::move(source));
    BOOST_TEST(moved.red() == 0.25f);
    BOOST_TEST(moved.green() == 0.5f);
    BOOST_TEST(moved.blue() == 0.75f);

    v3d::ecs::component::Color3 assigned(0.0f, 0.0f, 0.0f);
    v3d::ecs::component::Color3 other(0.9f, 0.8f, 0.7f);
    assigned = std::move(other);
    BOOST_TEST(assigned.red() == 0.9f);
    BOOST_TEST(assigned.green() == 0.8f);
    BOOST_TEST(assigned.blue() == 0.7f);
}

BOOST_AUTO_TEST_CASE(position1d_test) {
    v3d::ecs::component::Position1D position(3.5f);

    BOOST_TEST(position.x() == 3.5f);
    BOOST_TEST(position.value() == 3.5f);

    position.set(-1.25f);
    BOOST_TEST(position.x() == -1.25f);

    v3d::ecs::component::Position1D moved(std::move(position));
    BOOST_TEST(moved.value() == -1.25f);

    v3d::ecs::component::Position1D assigned(0.0f);
    assigned = std::move(moved);
    BOOST_TEST(assigned.value() == -1.25f);
}

BOOST_AUTO_TEST_CASE(position2d_test) {
    v3d::ecs::component::Position2D position(2.0f, -4.0f);

    BOOST_TEST(position.x() == 2.0f);
    BOOST_TEST(position.y() == -4.0f);
    BOOST_TEST(position.value().x == 2.0f);
    BOOST_TEST(position.value().y == -4.0f);

    position.set(glm::vec2(7.0f, 8.0f));
    BOOST_TEST(position.x() == 7.0f);
    BOOST_TEST(position.y() == 8.0f);

    v3d::ecs::component::Position2D moved(std::move(position));
    BOOST_TEST(moved.x() == 7.0f);
    BOOST_TEST(moved.y() == 8.0f);

    v3d::ecs::component::Position2D assigned(0.0f, 0.0f);
    assigned = std::move(moved);
    BOOST_TEST(assigned.x() == 7.0f);
    BOOST_TEST(assigned.y() == 8.0f);
}

/**
 * The fixed position is the one a tile is addressed by, so it holds integers and has no
 * set() - a mover replaces the component rather than writing through it.
 **/
BOOST_AUTO_TEST_CASE(position_fixed_2d_test) {
    v3d::ecs::component::PositionFixed2D position(5, -6);

    BOOST_TEST(position.x() == 5);
    BOOST_TEST(position.y() == -6);

    v3d::ecs::component::PositionFixed2D moved(std::move(position));
    BOOST_TEST(moved.x() == 5);
    BOOST_TEST(moved.y() == -6);

    v3d::ecs::component::PositionFixed2D assigned(0, 0);
    assigned = std::move(moved);
    BOOST_TEST(assigned.x() == 5);
    BOOST_TEST(assigned.y() == -6);
}

/**
 * A component is emplaced with the arguments its constructor takes and read back through the
 * registry, which is how every app holds one.
 **/
BOOST_AUTO_TEST_CASE(component_registry_test) {
    entt::registry registry;
    const entt::entity entity = registry.create();

    registry.emplace<v3d::ecs::component::Color3>(entity, 0.25f, 0.5f, 0.75f);
    registry.emplace<v3d::ecs::component::Position1D>(entity, 12.0f);

    BOOST_TEST(registry.get<v3d::ecs::component::Color3>(entity).green() == 0.5f);
    BOOST_TEST(registry.get<v3d::ecs::component::Position1D>(entity).value() == 12.0f);

    registry.get<v3d::ecs::component::Position1D>(entity).set(13.0f);
    BOOST_TEST(registry.get<v3d::ecs::component::Position1D>(entity).value() == 13.0f);

    BOOST_TEST(!registry.try_get<v3d::ecs::component::Position2D>(entity));
}
