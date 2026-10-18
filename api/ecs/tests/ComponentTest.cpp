/**
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 **/

#include <api/ecs/component/Color3.h>
#include <api/ecs/component/Position1D.h>
#include <api/ecs/component/Position2D.h>

#include <type_traits>

#include <boost/test/unit_test.hpp>

#include <entt/entt.hpp>
#include <glm/vec2.hpp>
#include <glm/vec3.hpp>

// a component is a value: an aggregate entt builds from the arguments to emplace, and a
// snapshot of the previous step is a copy
static_assert(std::is_aggregate_v<v3d::ecs::component::Color3>);
static_assert(std::is_aggregate_v<v3d::ecs::component::Position1D>);
static_assert(std::is_aggregate_v<v3d::ecs::component::Position2D>);
static_assert(std::is_copy_constructible_v<v3d::ecs::component::Color3>);
static_assert(std::is_copy_assignable_v<v3d::ecs::component::Position2D>);

namespace {

/**
 * A component left to its defaults is white, and at the origin.
 **/
BOOST_AUTO_TEST_CASE(component_defaults_test) {
    const v3d::ecs::component::Color3 color;
    BOOST_TEST(color.value.g == 1.0F);
    BOOST_TEST(v3d::ecs::component::Position1D().value == 0.0F);
    BOOST_TEST(v3d::ecs::component::Position2D().value.y == 0.0F);
}

/**
 * The positions blend linearly, as ecs::interpolated draws them between two steps.
 **/
BOOST_AUTO_TEST_CASE(component_interpolate_test) {
    const v3d::ecs::component::Position2D from{glm::vec2(0.0F, 10.0F)};
    const v3d::ecs::component::Position2D to{glm::vec2(4.0F, 20.0F)};
    const glm::vec2 quarter = interpolate(from, to, 0.25F).value;
    BOOST_TEST(quarter.x == 1.0F);
    BOOST_TEST(quarter.y == 12.5F);

    BOOST_TEST(interpolate(v3d::ecs::component::Position1D{2.0F}, v3d::ecs::component::Position1D{4.0F}, 0.5F).value == 3.0F);
}

/**
 * A component is emplaced with the value it holds and read back through the registry. Every
 * app holds its components this way.
 **/
BOOST_AUTO_TEST_CASE(component_registry_test) {
    entt::registry registry;
    const entt::entity entity = registry.create();

    registry.emplace<v3d::ecs::component::Color3>(entity, glm::vec3(0.25F, 0.5F, 0.75F));
    registry.emplace<v3d::ecs::component::Position1D>(entity, 12.0F);

    BOOST_TEST(registry.get<v3d::ecs::component::Color3>(entity).value.g == 0.5F);
    BOOST_TEST(registry.get<v3d::ecs::component::Position1D>(entity).value == 12.0F);

    registry.get<v3d::ecs::component::Position1D>(entity).value = 13.0F;
    BOOST_TEST(registry.get<v3d::ecs::component::Position1D>(entity).value == 13.0F);

    BOOST_TEST(!registry.try_get<v3d::ecs::component::Position2D>(entity));
}

};  // namespace
