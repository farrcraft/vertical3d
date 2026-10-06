/**
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 **/

#include <api/render/realtime/Shadow.h>
#include <api/render/realtime/component/Mesh.h>

#include <api/ecs/component/Transform.h>
#include <api/type/camera/Camera.h>

#include <cmath>
#include <optional>
#include <vector>

#include <boost/test/unit_test.hpp>
#include <entt/entt.hpp>
#include <glm/geometric.hpp>
#include <glm/mat4x4.hpp>
#include <glm/vec4.hpp>

using v3d::ecs::component::Transform;
using v3d::render::realtime::component::Mesh;

namespace shadow = v3d::render::realtime::shadow;

namespace {

/**
 * Where a world point lands in clip space, after the divide - which an orthographic matrix
 * leaves at one anyway.
 **/
glm::vec3 project(const glm::mat4& matrix, const glm::vec3& point) {
    const glm::vec4 clip = matrix * glm::vec4(point, 1.0f);
    return glm::vec3(clip) / clip.w;
}

void caster(entt::registry* registry, const glm::vec3& position, bool castsShadow) {
    const entt::entity entity = registry->create();
    Transform transform;
    transform.position = position;
    registry->emplace<Transform>(entity, transform);
    registry->emplace<Mesh>(entity, Mesh{v3d::render::realtime::MeshHandle(), castsShadow});
}

};  // namespace

BOOST_AUTO_TEST_SUITE(shadow_test)

/**
 * The eye stands two radii out and the far plane four beyond it, so the centre is in the middle
 * of the map at half depth, whatever the light's direction.
 **/
BOOST_AUTO_TEST_CASE(the_centre_is_the_middle_of_the_map_at_half_depth) {
    const glm::vec3 centre(3.0f, -1.0f, 7.0f);
    const glm::mat4 light = shadow::light(glm::vec3(-0.45f, 0.62f, 0.64f), centre, 5.0f);

    const glm::vec3 at = project(light, centre);
    BOOST_CHECK_SMALL(at.x, 1e-5f);
    BOOST_CHECK_SMALL(at.y, 1e-5f);
    BOOST_CHECK_CLOSE(at.z, 0.5f, 1e-3f);
}

/**
 * The box reaches a radius either side, so a point that far along the light's right is at the
 * map's edge, and one that far towards or away from the light is a quarter of the depth nearer
 * or further.
 **/
BOOST_AUTO_TEST_CASE(a_radius_out_is_the_edge_of_the_map) {
    const glm::vec3 towards = glm::normalize(glm::vec3(-0.45f, 0.62f, 0.64f));
    const glm::vec3 centre(3.0f, -1.0f, 7.0f);
    const float radius = 5.0f;
    const glm::mat4 light = shadow::light(towards, centre, radius);

    // right is up crossed into the view, as type::camera::Profile::lookat takes it
    const glm::vec3 right = glm::normalize(glm::cross(glm::vec3(0.0f, 1.0f, 0.0f), -towards));
    BOOST_CHECK_CLOSE(project(light, centre + right * radius).x, 1.0f, 1e-3f);
    BOOST_CHECK_CLOSE(project(light, centre - right * radius).x, -1.0f, 1e-3f);
    BOOST_CHECK_CLOSE(project(light, centre + towards * radius).z, 0.25f, 1e-3f);
    BOOST_CHECK_CLOSE(project(light, centre - towards * radius).z, 0.75f, 1e-3f);
}

/**
 * A light is drawn through the matrix a camera of the tree's own would build at the same eye,
 * looking at the same point with the same box. The shadow pass can then cull with the cel
 * pass's winding. A camera built any other way can turn every face around on screen.
 **/
BOOST_AUTO_TEST_CASE(the_light_sees_as_a_camera_does) {
    const glm::vec3 towards = glm::normalize(glm::vec3(-0.45f, 0.62f, 0.64f));
    const glm::vec3 centre(3.0f, -1.0f, 7.0f);
    const float radius = 5.0f;

    v3d::type::camera::Camera camera;
    v3d::type::camera::Profile& profile = camera.profile();
    profile.orthographic(true);
    profile.orthoZoom(radius);
    profile.pixelAspect(1.0f);
    profile.clipping(0.0f, radius * 4.0f);
    profile.eye(centre + towards * (radius * 2.0f));
    profile.up(glm::vec3(0.0f, 1.0f, 0.0f));
    profile.lookat(centre);
    camera.createProjection();
    camera.createView();

    const glm::mat4 expected = camera.projection() * camera.view();
    const glm::mat4 light = shadow::light(towards, centre, radius);
    for (int column = 0; column < 4; column++) {
        for (int row = 0; row < 4; row++) {
            BOOST_TEST_CONTEXT("[" << column << "][" << row << "]") {
                BOOST_CHECK_SMALL(light[column][row] - expected[column][row], 1e-5f);
            }
        }
    }
}

/**
 * A light straight overhead looks straight down, where the usual up has no right to cross
 * into. The matrix takes z as up instead, and is still a matrix rather than a set of NaNs.
 **/
BOOST_AUTO_TEST_CASE(a_light_overhead_takes_z_as_up) {
    const glm::vec3 centre(1.0f, 0.0f, 2.0f);
    const glm::mat4 light = shadow::light(glm::vec3(0.0f, 3.0f, 0.0f), centre, 2.0f);

    for (int column = 0; column < 4; column++) {
        for (int row = 0; row < 4; row++) {
            BOOST_CHECK(std::isfinite(light[column][row]));
        }
    }
    const glm::vec3 at = project(light, centre);
    BOOST_CHECK_SMALL(at.x, 1e-5f);
    BOOST_CHECK_SMALL(at.y, 1e-5f);
    BOOST_CHECK_CLOSE(at.z, 0.5f, 1e-3f);
    // z up crossed into a view along -y is +x, so the world's x is the map's right
    BOOST_CHECK_CLOSE(project(light, centre + glm::vec3(2.0f, 0.0f, 0.0f)).x, 1.0f, 1e-3f);
}

/**
 * The centre is the mean of the casters and the radius the farthest of them plus the margin.
 * The ground casts nothing, and an extra point to cover counts as a caster.
 **/
BOOST_AUTO_TEST_CASE(the_fit_covers_the_casters_and_not_the_ground) {
    entt::registry registry;
    caster(&registry, glm::vec3(0.0f, 0.0f, 0.0f), true);
    caster(&registry, glm::vec3(4.0f, 0.0f, 0.0f), true);
    caster(&registry, glm::vec3(2.0f, 0.0f, 6.0f), true);
    caster(&registry, glm::vec3(100.0f, 0.0f, 100.0f), false);

    const std::optional<shadow::Bounds> fitted = shadow::fit(registry, {}, 3.0f);
    const std::vector<glm::vec3> cover{glm::vec3(2.0f, 0.0f, -10.0f)};
    const std::optional<shadow::Bounds> covered = shadow::fit(registry, cover, 3.0f);
    if (!fitted || !covered) {
        BOOST_ERROR("three casters gave no fit");
        return;
    }

    BOOST_CHECK_CLOSE(fitted->centre.x, 2.0f, 1e-3f);
    BOOST_CHECK_SMALL(fitted->centre.y, 1e-5f);
    BOOST_CHECK_CLOSE(fitted->centre.z, 2.0f, 1e-3f);
    // (2, 0, 6) is four from the centre, and the other two are sqrt(8)
    BOOST_CHECK_CLOSE(fitted->radius, 7.0f, 1e-3f);
    BOOST_CHECK_CLOSE(covered->centre.z, -1.0f, 1e-3f);
}

/**
 * One caster with no margin has no spread. The radius is held at the minimum rather than zero,
 * so the light matrix built from it is finite.
 **/
BOOST_AUTO_TEST_CASE(one_caster_with_no_margin_fits_a_finite_light) {
    entt::registry registry;
    caster(&registry, glm::vec3(1.0f, 2.0f, 3.0f), true);

    const std::optional<shadow::Bounds> fitted = shadow::fit(registry, {}, 0.0f);
    const std::optional<shadow::Bounds> shrunk = shadow::fit(registry, {}, -1.0f);
    if (!fitted || !shrunk) {
        BOOST_ERROR("one caster gave no fit");
        return;
    }
    BOOST_CHECK_EQUAL(fitted->radius, shadow::minimumRadius);
    BOOST_CHECK_EQUAL(shrunk->radius, shadow::minimumRadius);

    const glm::mat4 light = shadow::light(glm::vec3(-0.45f, 0.62f, 0.64f), fitted->centre, fitted->radius);
    for (int column = 0; column < 4; column++) {
        for (int row = 0; row < 4; row++) {
            BOOST_CHECK(std::isfinite(light[column][row]));
        }
    }
}

/**
 * With nothing casting there is nothing to fit, and the caller keeps the bounds it had rather
 * than being handed a sphere of radius zero.
 **/
BOOST_AUTO_TEST_CASE(nothing_casting_fits_nothing) {
    entt::registry registry;
    caster(&registry, glm::vec3(1.0f), false);
    const std::vector<glm::vec3> cover{glm::vec3(0.0f)};

    BOOST_CHECK(!shadow::fit(registry, cover, 3.0f));
    BOOST_CHECK(!shadow::fit(entt::registry(), {}, 3.0f));
}

BOOST_AUTO_TEST_SUITE_END()
