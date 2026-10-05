/**
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 **/

#include <api/ecs/Previous.h>
#include <api/ecs/component/Position1D.h>
#include <api/ecs/component/Position2D.h>

#include <boost/test/unit_test.hpp>

#include <entt/entt.hpp>
#include <glm/vec2.hpp>

namespace {

/** A tile address, which has no interpolate() and so no position between two of it. **/
struct Tile {
    int x = 0;
};

};  // namespace

// the positions can be drawn between steps, and a tile cannot: interpolated<Tile> fails to
// compile rather than snapping
static_assert(v3d::ecs::Interpolable<v3d::ecs::component::Position1D>);
static_assert(v3d::ecs::Interpolable<v3d::ecs::component::Position2D>);
static_assert(!v3d::ecs::Interpolable<Tile>);

/**
 * A snapshot then a move is drawn at the snapshot at alpha 0, at the move at alpha 1, and
 * between the two in between.
 **/
BOOST_AUTO_TEST_CASE(previous_interpolated_test) {
    entt::registry registry;
    const entt::entity entity = registry.create();
    registry.emplace<v3d::ecs::component::Position2D>(entity, glm::vec2(10.0f, 20.0f));

    v3d::ecs::snapshot<v3d::ecs::component::Position2D>(registry);
    registry.get<v3d::ecs::component::Position2D>(entity).value = glm::vec2(30.0f, 60.0f);

    const glm::vec2 start = v3d::ecs::interpolated<v3d::ecs::component::Position2D>(registry, entity, 0.0f).value;
    const glm::vec2 end = v3d::ecs::interpolated<v3d::ecs::component::Position2D>(registry, entity, 1.0f).value;
    const glm::vec2 quarter = v3d::ecs::interpolated<v3d::ecs::component::Position2D>(registry, entity, 0.25f).value;

    BOOST_TEST((start == glm::vec2(10.0f, 20.0f)));
    BOOST_TEST((end == glm::vec2(30.0f, 60.0f)));
    BOOST_TEST((quarter == glm::vec2(15.0f, 30.0f)));
}

/**
 * A snapshot replaces the previous one rather than keeping the first, so what is blended is
 * always the last two steps.
 **/
BOOST_AUTO_TEST_CASE(previous_snapshot_replaces_test) {
    entt::registry registry;
    const entt::entity entity = registry.create();
    registry.emplace<v3d::ecs::component::Position1D>(entity, 0.0f);

    v3d::ecs::snapshot<v3d::ecs::component::Position1D>(registry);
    registry.get<v3d::ecs::component::Position1D>(entity).value = 4.0f;
    v3d::ecs::snapshot<v3d::ecs::component::Position1D>(registry);
    registry.get<v3d::ecs::component::Position1D>(entity).value = 8.0f;

    BOOST_TEST(v3d::ecs::interpolated<v3d::ecs::component::Position1D>(registry, entity, 0.5f).value == 6.0f);
}

/**
 * An entity created after the snapshot has no previous step, and is drawn where it is.
 **/
BOOST_AUTO_TEST_CASE(previous_created_after_the_snapshot_test) {
    entt::registry registry;
    v3d::ecs::snapshot<v3d::ecs::component::Position2D>(registry);

    const entt::entity entity = registry.create();
    registry.emplace<v3d::ecs::component::Position2D>(entity, glm::vec2(5.0f, 7.0f));

    BOOST_TEST(!registry.all_of<v3d::ecs::Previous<v3d::ecs::component::Position2D>>(entity));
    const glm::vec2 drawn = v3d::ecs::interpolated<v3d::ecs::component::Position2D>(registry, entity, 0.5f).value;
    BOOST_TEST((drawn == glm::vec2(5.0f, 7.0f)));
}

/**
 * A settle after a move draws no motion, as a teleport needs.
 **/
BOOST_AUTO_TEST_CASE(previous_settle_test) {
    entt::registry registry;
    const entt::entity entity = registry.create();
    registry.emplace<v3d::ecs::component::Position2D>(entity, glm::vec2(0.0f, 0.0f));

    v3d::ecs::snapshot<v3d::ecs::component::Position2D>(registry);
    registry.get<v3d::ecs::component::Position2D>(entity).value = glm::vec2(400.0f, 300.0f);
    v3d::ecs::settle<v3d::ecs::component::Position2D>(registry, entity);

    const glm::vec2 drawn = v3d::ecs::interpolated<v3d::ecs::component::Position2D>(registry, entity, 0.25f).value;
    BOOST_TEST((drawn == glm::vec2(400.0f, 300.0f)));
}

/**
 * The snapshot covers every entity carrying the type, and leaves an entity that does not
 * carry it alone.
 **/
BOOST_AUTO_TEST_CASE(previous_snapshot_covers_every_entity_test) {
    entt::registry registry;
    const entt::entity first = registry.create();
    const entt::entity second = registry.create();
    const entt::entity other = registry.create();
    registry.emplace<v3d::ecs::component::Position1D>(first, 1.0f);
    registry.emplace<v3d::ecs::component::Position1D>(second, 2.0f);
    registry.emplace<v3d::ecs::component::Position2D>(other, glm::vec2(0.0f, 0.0f));

    v3d::ecs::snapshot<v3d::ecs::component::Position1D>(registry);

    BOOST_TEST(registry.get<v3d::ecs::Previous<v3d::ecs::component::Position1D>>(first).value.value == 1.0f);
    BOOST_TEST(registry.get<v3d::ecs::Previous<v3d::ecs::component::Position1D>>(second).value.value == 2.0f);
    BOOST_TEST(!registry.all_of<v3d::ecs::Previous<v3d::ecs::component::Position1D>>(other));
}
