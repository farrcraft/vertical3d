/**
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 **/

#include <api/ecs/Ref.h>

#include <boost/test/unit_test.hpp>

#include <entt/entt.hpp>

/**
 * A reference to a destroyed entity reads null after EnTT has reused its index. The case
 * creates entities until one lands on the destroyed index, so a reference that compared only
 * the index would read that new entity.
 **/
BOOST_AUTO_TEST_CASE(ref_reads_null_after_the_index_is_reused_test) {
    entt::registry registry;
    const entt::entity held = registry.create();
    v3d::ecs::Ref ref(held);
    BOOST_TEST((ref.get(registry) == held));

    registry.destroy(held);
    entt::entity reused = entt::null;
    for (int created = 0; created < 16 && reused == entt::null; created++) {
        const entt::entity entity = registry.create();
        if (entt::to_entity(entity) == entt::to_entity(held)) {
            reused = entity;
        }
    }
    BOOST_REQUIRE_MESSAGE(reused != entt::null, "EnTT never reused the destroyed index");
    BOOST_TEST((reused != held));

    BOOST_TEST((ref.get(registry) == entt::null));
}

/**
 * set() holds a new entity, and clear() and a default Ref hold none.
 **/
BOOST_AUTO_TEST_CASE(ref_set_and_clear_test) {
    entt::registry registry;
    const entt::entity first = registry.create();
    const entt::entity second = registry.create();

    v3d::ecs::Ref ref;
    BOOST_TEST((ref.get(registry) == entt::null));

    ref.set(first);
    BOOST_TEST((ref.get(registry) == first));
    ref.set(second);
    BOOST_TEST((ref.get(registry) == second));
    ref.clear();
    BOOST_TEST((ref.get(registry) == entt::null));
}
