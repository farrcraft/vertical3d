/**
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 **/

#include <api/ecs/component/Emitter.h>
#include <api/ecs/component/Transform.h>

#include <boost/test/unit_test.hpp>

#include <entt/entt.hpp>

using v3d::ecs::component::Emitter;
using v3d::ecs::component::Transform;

namespace {

BOOST_AUTO_TEST_SUITE(emitter_test)

/**
 * An emitter spawns where its entity's transform stands, and follows the entity when it moves.
 **/
BOOST_AUTO_TEST_CASE(emitter_spawns_where_its_entity_stands_test) {
    entt::registry registry;
    const entt::entity torch = registry.create();
    registry.emplace<Transform>(torch).position = glm::vec3(3.0F, 0.0F, -2.0F);
    Emitter& emitter = registry.emplace<Emitter>(torch, 7U);
    emitter.description.rate = 60.0F;
    emitter.description.lifeMin = 10.0F;
    emitter.description.lifeMax = 10.0F;

    v3d::ecs::component::emit(registry, 1.0F / 60.0F);
    BOOST_REQUIRE_EQUAL(emitter.state.particles.size(), 1U);
    BOOST_CHECK(emitter.state.particles[0].position == glm::vec3(3.0F, 0.0F, -2.0F));

    registry.get<Transform>(torch).position = glm::vec3(-1.0F, 1.0F, 0.0F);
    v3d::ecs::component::emit(registry, 1.0F / 60.0F);
    BOOST_REQUIRE_EQUAL(emitter.state.particles.size(), 2U);
    BOOST_CHECK(emitter.state.particles[1].position == glm::vec3(-1.0F, 1.0F, 0.0F));
    // the first stays where it was born
    BOOST_CHECK(emitter.state.particles[0].position == glm::vec3(3.0F, 0.0F, -2.0F));
}

/**
 * An emitter on an entity with no transform stands at the origin.
 **/
BOOST_AUTO_TEST_CASE(emitter_without_a_transform_test) {
    entt::registry registry;
    Emitter& emitter = registry.emplace<Emitter>(registry.create());
    emitter.description.rate = 60.0F;

    v3d::ecs::component::emit(registry, 1.0F / 60.0F);
    BOOST_REQUIRE_EQUAL(emitter.state.particles.size(), 1U);
    BOOST_CHECK(emitter.state.particles[0].position == glm::vec3(0.0F));
}

BOOST_AUTO_TEST_SUITE_END()

};  // namespace
