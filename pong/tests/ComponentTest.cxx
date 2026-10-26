/**
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 **/

#include <pong/src/component/Color3.h>
#include <pong/src/component/Position1D.h>
#include <pong/src/component/Position2D.h>

#include <type_traits>

#include <boost/test/unit_test.hpp>

#include <entt/entt.hpp>
#include <glm/vec2.hpp>
#include <glm/vec3.hpp>

// each component is a copyable aggregate, so entt builds it from the arguments to emplace, and
// snapshot() copies it
static_assert(std::is_aggregate_v<Color3>);
static_assert(std::is_aggregate_v<Position1D>);
static_assert(std::is_aggregate_v<Position2D>);
static_assert(std::is_copy_constructible_v<Color3>);
static_assert(std::is_copy_assignable_v<Position2D>);

namespace {

/**
 * A component left to its defaults is white, and at the origin.
 **/
BOOST_AUTO_TEST_CASE(component_defaults_test) {
    const Color3 color;
    BOOST_TEST(color.value.g == 1.0F);
    BOOST_TEST(Position1D().value == 0.0F);
    BOOST_TEST(Position2D().value.y == 0.0F);
}

/**
 * The positions blend linearly, as ecs::interpolated draws them between two steps.
 **/
BOOST_AUTO_TEST_CASE(component_interpolate_test) {
    const Position2D from{glm::vec2(0.0F, 10.0F)};
    const Position2D to{glm::vec2(4.0F, 20.0F)};
    const glm::vec2 quarter = interpolate(from, to, 0.25F).value;
    BOOST_TEST(quarter.x == 1.0F);
    BOOST_TEST(quarter.y == 12.5F);

    BOOST_TEST(interpolate(Position1D{2.0F}, Position1D{4.0F}, 0.5F).value == 3.0F);
}

/**
 * A component is emplaced with the value it holds, and read back and written through the
 * registry.
 **/
BOOST_AUTO_TEST_CASE(component_registry_test) {
    entt::registry registry;
    const entt::entity entity = registry.create();

    registry.emplace<Color3>(entity, glm::vec3(0.25F, 0.5F, 0.75F));
    registry.emplace<Position1D>(entity, 12.0F);

    BOOST_TEST(registry.get<Color3>(entity).value.g == 0.5F);
    BOOST_TEST(registry.get<Position1D>(entity).value == 12.0F);

    registry.get<Position1D>(entity).value = 13.0F;
    BOOST_TEST(registry.get<Position1D>(entity).value == 13.0F);

    BOOST_TEST(!registry.try_get<Position2D>(entity));
}

};  // namespace
