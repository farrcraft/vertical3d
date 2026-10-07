/**
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 **/

#include <api/ecs/Previous.h>
#include <api/ecs/component/Transform.h>

#include <cmath>
#include <limits>

#include <boost/test/unit_test.hpp>

#include <entt/entt.hpp>
#include <glm/gtc/constants.hpp>
#include <glm/gtc/epsilon.hpp>

static_assert(v3d::ecs::Interpolable<v3d::ecs::component::Transform>);

namespace {

bool same(const glm::vec3& a, const glm::vec3& b) {
    return glm::all(glm::epsilonEqual(a, b, 0.0001f));
}

};  // namespace

/**
 * A point is scaled, then turned, then moved: (1, 0, 0) doubled, a quarter turn about Y, and
 * moved up by 5.
 **/
BOOST_AUTO_TEST_CASE(transform_matrix_order_test) {
    v3d::ecs::component::Transform transform;
    transform.position = glm::vec3(0.0f, 5.0f, 0.0f);
    transform.rotation = v3d::ecs::component::aboutY(glm::half_pi<float>());
    transform.scale = glm::vec3(2.0f);

    const glm::vec3 moved(transform.matrix() * glm::vec4(1.0f, 0.0f, 0.0f, 1.0f));

    BOOST_TEST(same(moved, glm::vec3(0.0f, 5.0f, -2.0f)));
}

/**
 * The hand of aboutY: a positive angle turns +Z towards +X, as glm::rotate about +Y does.
 **/
BOOST_AUTO_TEST_CASE(transform_about_y_hand_test) {
    const glm::vec3 turned = v3d::ecs::component::aboutY(glm::half_pi<float>()) * glm::vec3(0.0f, 0.0f, 1.0f);

    BOOST_TEST(same(turned, glm::vec3(1.0f, 0.0f, 0.0f)));
}

/**
 * Position and scale are mixed, and the ends are the ends.
 **/
BOOST_AUTO_TEST_CASE(transform_interpolate_test) {
    v3d::ecs::component::Transform from;
    v3d::ecs::component::Transform to;
    to.position = glm::vec3(4.0f, 0.0f, -8.0f);
    to.scale = glm::vec3(3.0f);
    to.rotation = v3d::ecs::component::aboutY(1.0f);

    const v3d::ecs::component::Transform start = v3d::ecs::component::interpolate(from, to, 0.0f);
    const v3d::ecs::component::Transform end = v3d::ecs::component::interpolate(from, to, 1.0f);
    const v3d::ecs::component::Transform half = v3d::ecs::component::interpolate(from, to, 0.5f);

    BOOST_TEST(same(start.position, from.position));
    BOOST_TEST(same(end.position, to.position));
    BOOST_TEST(same(half.position, glm::vec3(2.0f, 0.0f, -4.0f)));
    BOOST_TEST(same(half.scale, glm::vec3(2.0f)));
    BOOST_TEST(same(end.rotation * glm::vec3(0.0f, 0.0f, 1.0f), to.rotation * glm::vec3(0.0f, 0.0f, 1.0f)));
}

/**
 * Halfway from 170 degrees to -170 degrees is 180, the 20 degree way round. An angle lerped as
 * a float gives 0, having swept 340 degrees the other way.
 **/
BOOST_AUTO_TEST_CASE(transform_interpolate_short_way_test) {
    v3d::ecs::component::Transform from;
    v3d::ecs::component::Transform to;
    from.rotation = v3d::ecs::component::aboutY(glm::radians(170.0f));
    to.rotation = v3d::ecs::component::aboutY(glm::radians(-170.0f));

    const glm::vec3 facing = v3d::ecs::component::interpolate(from, to, 0.5f).rotation * glm::vec3(0.0f, 0.0f, 1.0f);

    BOOST_TEST(same(facing, glm::vec3(0.0f, 0.0f, -1.0f)));
}

/**
 * Drawn between steps through ecs::interpolated, like any other interpolable component.
 **/
BOOST_AUTO_TEST_CASE(transform_interpolated_test) {
    entt::registry registry;
    const entt::entity entity = registry.create();
    registry.emplace<v3d::ecs::component::Transform>(entity);

    v3d::ecs::snapshot<v3d::ecs::component::Transform>(registry);
    registry.get<v3d::ecs::component::Transform>(entity).position = glm::vec3(0.0f, 0.0f, 10.0f);

    const v3d::ecs::component::Transform quarter =
        v3d::ecs::interpolated<v3d::ecs::component::Transform>(registry, entity, 0.25f);

    BOOST_TEST(same(quarter.position, glm::vec3(0.0f, 0.0f, 2.5f)));
}

/**
 * A rotation that has drifted off unit length still only turns: the matrix is built from it
 * normalised, so it neither scales nor shears.
 **/
BOOST_AUTO_TEST_CASE(transform_a_drifted_rotation_only_turns_test) {
    v3d::ecs::component::Transform transform;
    transform.rotation = v3d::ecs::component::aboutY(glm::half_pi<float>()) * 1.5f;

    const glm::vec3 moved(transform.matrix() * glm::vec4(1.0f, 0.0f, 0.0f, 1.0f));
    BOOST_TEST(same(moved, glm::vec3(0.0f, 0.0f, -1.0f)));
}

/**
 * A rotation of no finite length is treated as no rotation, rather than turning the matrix into
 * NaNs.
 **/
BOOST_AUTO_TEST_CASE(transform_an_infinite_rotation_is_no_rotation_test) {
    v3d::ecs::component::Transform transform;
    const float inf = std::numeric_limits<float>::infinity();
    transform.rotation = glm::quat(inf, 0.0f, 0.0f, 0.0f);
    const glm::vec3 moved(transform.matrix() * glm::vec4(1.0f, 0.0f, 0.0f, 1.0f));
    BOOST_TEST(same(moved, glm::vec3(1.0f, 0.0f, 0.0f)));
}
