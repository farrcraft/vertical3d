/**
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 **/

#include <api/render/realtime/lighting/Shadow.h>
#include <api/render/realtime/component/Mesh.h>

#include <api/ecs/Previous.h>
#include <api/ecs/component/Transform.h>
#include <api/type/camera/Camera.h>

#include <cmath>
#include <cstdint>
#include <limits>
#include <optional>
#include <vector>

#include <boost/test/unit_test.hpp>
#include <entt/entt.hpp>
#include <glm/geometric.hpp>
#include <glm/mat4x4.hpp>
#include <glm/matrix.hpp>
#include <glm/vec2.hpp>
#include <glm/vec4.hpp>

using v3d::ecs::component::Transform;
using v3d::render::realtime::component::Mesh;

namespace lighting = v3d::render::realtime::lighting;

namespace {

/**
 * Where a world point lands in clip space, after the divide - which an orthographic matrix
 * leaves at one anyway.
 **/
glm::vec3 project(const glm::mat4& matrix, const glm::vec3& point) {
    const glm::vec4 clip = matrix * glm::vec4(point, 1.0f);
    return glm::vec3(clip) / clip.w;
}

/**
 * Check lighting::shadowLight against the matrix a camera of the given hand builds at the
 * light's eye, looking at the same centre through the same box.
 **/
void checkSeenAsCamera(v3d::type::camera::Profile::Hand hand) {
    const glm::vec3 towards = glm::normalize(glm::vec3(-0.45f, 0.62f, 0.64f));
    const glm::vec3 centre(3.0f, -1.0f, 7.0f);
    const float radius = 5.0f;

    v3d::type::camera::Camera camera;
    v3d::type::camera::Profile& profile = camera.profile();
    profile.hand(hand);
    profile.orthographic(true);
    profile.orthoZoom(radius);
    profile.pixelAspect(1.0f);
    profile.clipping(glm::vec2(0.0f, radius * 4.0f));
    profile.eye(centre + towards * (radius * 2.0f));
    profile.up(glm::vec3(0.0f, 1.0f, 0.0f));
    profile.lookat(centre);
    camera.createProjection();
    camera.createView();

    const glm::mat4 expected = camera.projection() * camera.view();
    const glm::mat4 light = lighting::shadowLight(towards, {.centre = centre, .radius = radius}, hand);
    for (int column = 0; column < 4; column++) {
        for (int row = 0; row < 4; row++) {
            BOOST_TEST_CONTEXT("[" << column << "][" << row << "]") {
                BOOST_CHECK_SMALL(light[column][row] - expected[column][row], 1e-5f);
            }
        }
    }
}

void caster(entt::registry* registry, const glm::vec3& position, bool castsShadow) {
    const entt::entity entity = registry->create();
    Transform transform;
    transform.position = position;
    registry->emplace<Transform>(entity, transform);
    registry->emplace<Mesh>(entity, Mesh{v3d::render::realtime::MeshHandle(), castsShadow});
}

/**
 * The direction towards a light low in the sky, for the view fit's cases.
 **/
glm::vec3 towards() {
    return glm::normalize(glm::vec3(-0.45f, 0.62f, 0.64f));
}

/** Where a camera is, the point it looks at, and how far it sees. **/
struct Looking {
    glm::vec3 eye{0.0f};
    glm::vec3 at{0.0f};
    float farPlane = 100.0f;
};

/**
 * A perspective camera's view projection, looking from an eye at a point.
 **/
glm::mat4 perspective(const Looking& looking) {
    const glm::vec3& eye = looking.eye;
    const glm::vec3& at = looking.at;
    const float farPlane = looking.farPlane;
    v3d::type::camera::Camera camera;
    v3d::type::camera::Profile& profile = camera.profile();
    profile.orthographic(false);
    profile.fov(60.0f);
    profile.pixelAspect(1.0f);
    profile.clipping(glm::vec2(0.5f, farPlane));
    profile.eye(eye);
    profile.up(glm::vec3(0.0f, 1.0f, 0.0f));
    profile.lookat(at);
    camera.createProjection();
    camera.createView();
    return camera.projection() * camera.view();
}

/**
 * The corners of a camera's view volume, each far corner cut at a distance along its edge.
 **/
std::vector<glm::vec3> viewCorners(const glm::mat4& viewProjection, float distance) {
    const glm::mat4 inverse = glm::inverse(viewProjection);
    std::vector<glm::vec3> corners;
    for (const float x : {-1.0f, 1.0f}) {
        for (const float y : {-1.0f, 1.0f}) {
            const glm::vec4 nearClip = inverse * glm::vec4(x, y, 0.0f, 1.0f);
            const glm::vec4 farClip = inverse * glm::vec4(x, y, 1.0f, 1.0f);
            const glm::vec3 nearCorner = glm::vec3(nearClip) / nearClip.w;
            const glm::vec3 farCorner = glm::vec3(farClip) / farClip.w;
            const glm::vec3 edge = farCorner - nearCorner;
            corners.push_back(nearCorner);
            corners.push_back(glm::length(edge) > distance ? nearCorner + glm::normalize(edge) * distance : farCorner);
        }
    }
    return corners;
}

};  // namespace

BOOST_AUTO_TEST_SUITE(shadow_test)

/**
 * The eye stands two radii out and the far plane four beyond it, so the centre is in the middle
 * of the map at half depth, whatever the light's direction.
 **/
BOOST_AUTO_TEST_CASE(the_centre_is_the_middle_of_the_map_at_half_depth) {
    const glm::vec3 centre(3.0f, -1.0f, 7.0f);
    const glm::mat4 light = lighting::shadowLight(glm::vec3(-0.45f, 0.62f, 0.64f), {.centre = centre, .radius = 5.0f});

    const glm::vec3 at = project(light, centre);
    BOOST_CHECK_SMALL(at.x, 1e-5f);
    BOOST_CHECK_SMALL(at.y, 1e-5f);
    BOOST_CHECK_CLOSE(at.z, 0.5f, 1e-3f);
}

/**
 * The box reaches a radius either side, so a point that far along the light's right is at the
 * map's edge. A point that far towards or away from the light is a quarter of the depth nearer
 * or further.
 **/
BOOST_AUTO_TEST_CASE(a_radius_out_is_the_edge_of_the_map) {
    const glm::vec3 towards = glm::normalize(glm::vec3(-0.45f, 0.62f, 0.64f));
    const glm::vec3 centre(3.0f, -1.0f, 7.0f);
    const float radius = 5.0f;
    const glm::mat4 light = lighting::shadowLight(towards, {.centre = centre, .radius = radius});

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
    checkSeenAsCamera(v3d::type::camera::Profile::Hand::UpCrossDirection);
}

/**
 * Under the mirrored hand the light still matches a camera of that hand. Its right is negated
 * and its up kept, so a face keeps the winding the camera gives it. A light that ignored the hand
 * would match only the case above.
 *
 * Covers the base: 7d680422 gave lighting::shadowLight() the hand. Checked by keeping the right for
 * both hands, which fails this case.
 **/
BOOST_AUTO_TEST_CASE(the_light_sees_as_a_mirrored_camera_does) {
    checkSeenAsCamera(v3d::type::camera::Profile::Hand::DirectionCrossUp);
}

/**
 * A light straight overhead looks straight down, where the usual up has no right to cross
 * into. The matrix takes z as up instead, and is still a matrix rather than a set of NaNs.
 **/
BOOST_AUTO_TEST_CASE(a_light_overhead_takes_z_as_up) {
    const glm::vec3 centre(1.0f, 0.0f, 2.0f);
    const glm::mat4 light = lighting::shadowLight(glm::vec3(0.0f, 3.0f, 0.0f), {.centre = centre, .radius = 2.0f});

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

    const std::optional<lighting::ShadowBounds> fitted = lighting::fitShadow(registry, {}, {.margin = 3.0f});
    const std::vector<glm::vec3> cover{glm::vec3(2.0f, 0.0f, -10.0f)};
    const std::optional<lighting::ShadowBounds> covered = lighting::fitShadow(registry, cover, {.margin = 3.0f});
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

    const std::optional<lighting::ShadowBounds> fitted = lighting::fitShadow(registry, {}, {.margin = 0.0f});
    const std::optional<lighting::ShadowBounds> shrunk = lighting::fitShadow(registry, {}, {.margin = -1.0f});
    if (!fitted || !shrunk) {
        BOOST_ERROR("one caster gave no fit");
        return;
    }
    BOOST_CHECK_EQUAL(fitted->radius, lighting::minimumShadowRadius);
    BOOST_CHECK_EQUAL(shrunk->radius, lighting::minimumShadowRadius);

    const glm::mat4 light = lighting::shadowLight(glm::vec3(-0.45f, 0.62f, 0.64f), *fitted);
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

    BOOST_CHECK(!lighting::fitShadow(registry, cover, {.margin = 3.0f}));
    BOOST_CHECK(!lighting::fitShadow(entt::registry(), {}, {.margin = 3.0f}));
}

/**
 * A caster is fitted where it is drawn, alpha of the way from its previous step, and at an
 * alpha of one where its current step puts it.
 **/
BOOST_AUTO_TEST_CASE(the_fit_reads_the_drawn_position) {
    entt::registry registry;
    caster(&registry, glm::vec3(0.0f, 0.0f, 0.0f), true);
    v3d::ecs::snapshot<Transform>(registry);
    for (auto [entity, transform] : registry.view<Transform>().each()) {
        transform.position = glm::vec3(10.0f, 0.0f, 0.0f);
    }

    const std::optional<lighting::ShadowBounds> halfway = lighting::fitShadow(registry, {}, {.margin = 1.0f, .alpha = 0.5f});
    const std::optional<lighting::ShadowBounds> current = lighting::fitShadow(registry, {}, {.margin = 1.0f});
    if (!halfway || !current) {
        BOOST_ERROR("a moving caster gave no fit");
        return;
    }
    BOOST_CHECK_CLOSE(halfway->centre.x, 5.0f, 1e-3f);
    BOOST_CHECK_CLOSE(current->centre.x, 10.0f, 1e-3f);
}

/**
 * The view fit encloses the camera's view volume out to the distance, and stops at the far
 * plane when that is nearer.
 **/
BOOST_AUTO_TEST_CASE(the_view_fit_encloses_what_the_camera_sees) {
    const glm::mat4 camera = perspective({.eye = glm::vec3(3.0f, 10.0f, 12.0f), .at = glm::vec3(0.0f), .farPlane = 200.0f});
    const std::optional<lighting::ShadowBounds> fitted = lighting::fitShadowToView(camera, 30.0f, towards(), 1024);
    if (!fitted) {
        BOOST_ERROR("a view gave no fit");
        return;
    }

    for (const glm::vec3& corner : viewCorners(camera, 30.0f)) {
        BOOST_CHECK_LE(glm::length(corner - fitted->centre), fitted->radius);
    }

    // a far plane nearer than the distance bounds the volume instead
    const glm::mat4 shallow = perspective({.eye = glm::vec3(3.0f, 10.0f, 12.0f), .at = glm::vec3(0.0f), .farPlane = 20.0f});
    const std::optional<lighting::ShadowBounds> cut = lighting::fitShadowToView(shallow, 30.0f, towards(), 1024);
    const std::optional<lighting::ShadowBounds> farther = lighting::fitShadowToView(camera, 300.0f, towards(), 1024);
    if (!cut || !farther) {
        BOOST_ERROR("a view gave no fit");
        return;
    }
    BOOST_CHECK_LT(cut->radius, fitted->radius);
    BOOST_CHECK_GT(farther->radius, fitted->radius);
}

/**
 * The radius stays the same as the camera moves and turns, so the map's texels stay the same
 * size.
 **/
BOOST_AUTO_TEST_CASE(the_view_fit_keeps_its_radius_as_the_camera_moves) {
    const std::optional<lighting::ShadowBounds> here = lighting::fitShadowToView(perspective({.eye = glm::vec3(3.0f, 10.0f, 12.0f), .at = glm::vec3(0.0f), .farPlane = 200.0f}),
        30.0f, towards(), 1024);
    const std::optional<lighting::ShadowBounds> moved = lighting::fitShadowToView(
        perspective({.eye = glm::vec3(-40.0f, 13.0f, 7.0f), .at = glm::vec3(-20.0f, 0.0f, 31.0f), .farPlane = 200.0f}), 30.0f, towards(), 1024);
    if (!here || !moved) {
        BOOST_ERROR("a view gave no fit");
        return;
    }
    BOOST_CHECK_CLOSE(here->radius, moved->radius, 1e-2f);
}

/**
 * A point fixed in the world falls at the same place within its texel however far the camera
 * slides, so a shadow's edge does not crawl. The light may be built in either hand.
 **/
BOOST_AUTO_TEST_CASE(the_view_fit_keeps_the_world_on_its_texels) {
    const uint32_t texels = 1024;
    const glm::vec3 point(1.3f, 0.0f, -2.7f);
    for (const v3d::type::camera::Profile::Hand hand :
        {v3d::type::camera::Profile::Hand::UpCrossDirection, v3d::type::camera::Profile::Hand::DirectionCrossUp}) {
        std::vector<glm::vec2> within;
        for (const float slide : {0.0f, 0.013f, 0.37f, 2.9f}) {
            const glm::vec3 offset(slide, 0.0f, slide * 0.5f);
            const glm::mat4 camera = perspective({.eye = glm::vec3(3.0f, 10.0f, 12.0f) + offset, .at = offset, .farPlane = 200.0f});
            const std::optional<lighting::ShadowBounds> fitted = lighting::fitShadowToView(camera, 30.0f, towards(), texels);
            if (!fitted) {
                BOOST_ERROR("a view gave no fit");
                return;
            }
            const glm::vec3 clip = project(lighting::shadowLight(towards(), *fitted, hand), point);
            const glm::vec2 texel = (glm::vec2(clip) + 1.0f) * 0.5f * static_cast<float>(texels);
            within.emplace_back(texel.x - std::floor(texel.x), texel.y - std::floor(texel.y));
        }
        for (const glm::vec2& fraction : within) {
            BOOST_CHECK_SMALL(fraction.x - within.front().x, 2e-2f);
            BOOST_CHECK_SMALL(fraction.y - within.front().y, 2e-2f);
        }
    }
}

/**
 * A distance, a direction, a map width or a matrix that cannot be fitted gives nothing.
 **/
BOOST_AUTO_TEST_CASE(the_view_fit_refuses_what_it_cannot_fit) {
    const glm::mat4 camera = perspective({.eye = glm::vec3(3.0f, 10.0f, 12.0f), .at = glm::vec3(0.0f), .farPlane = 200.0f});
    const float nan = std::numeric_limits<float>::quiet_NaN();

    BOOST_CHECK(!lighting::fitShadowToView(camera, 0.0f, towards(), 1024));
    BOOST_CHECK(!lighting::fitShadowToView(camera, nan, towards(), 1024));
    BOOST_CHECK(!lighting::fitShadowToView(camera, std::numeric_limits<float>::infinity(), towards(), 1024));
    BOOST_CHECK(!lighting::fitShadowToView(camera, 30.0f, glm::vec3(0.0f), 1024));
    BOOST_CHECK(!lighting::fitShadowToView(camera, 30.0f, glm::vec3(nan, 1.0f, 0.0f), 1024));
    BOOST_CHECK(!lighting::fitShadowToView(camera, 30.0f, towards(), 3));
    BOOST_CHECK(!lighting::fitShadowToView(glm::mat4(0.0f), 30.0f, towards(), 1024));
}

BOOST_AUTO_TEST_SUITE_END()
