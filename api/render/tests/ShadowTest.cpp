/**
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 **/

#include <api/render/realtime/lighting/SceneUniforms.h>
#include <api/render/realtime/lighting/Settings.h>
#include <api/render/realtime/lighting/Shadow.h>
#include <api/render/realtime/component/Mesh.h>

#include <api/ecs/Previous.h>
#include <api/ecs/component/Transform.h>
#include <api/type/camera/Camera.h>
#include <api/type/camera/Profile.h>

#include <cmath>
#include <cstddef>
#include <cstdint>
#include <limits>
#include <optional>
#include <vector>

#include <boost/test/unit_test.hpp>
#include <entt/entt.hpp>
#include <glm/ext/matrix_clip_space.hpp>
#include <glm/ext/matrix_transform.hpp>
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
 * Where a world point lands in clip space, after the perspective divide. An orthographic matrix
 * leaves w at one, so the divide changes nothing for it.
 **/
glm::vec3 project(const glm::mat4& matrix, const glm::vec3& point) {
    const glm::vec4 clip = matrix * glm::vec4(point, 1.0F);
    return glm::vec3(clip) / clip.w;
}

/**
 * Check lighting::shadowLight against the matrix a camera of the given hand builds at the
 * light's eye, looking at the same centre through the same box.
 **/
void checkSeenAsCamera(v3d::type::camera::Profile::Hand hand) {
    const glm::vec3 towards = glm::normalize(glm::vec3(-0.45F, 0.62F, 0.64F));
    const glm::vec3 centre(3.0F, -1.0F, 7.0F);
    const float radius = 5.0F;

    v3d::type::camera::Camera camera;
    v3d::type::camera::Profile& profile = camera.profile();
    profile.hand(hand);
    profile.orthographic(true);
    profile.orthoZoom(radius);
    profile.pixelAspect(1.0F);
    profile.clipping(glm::vec2(0.0F, radius * 4.0F));
    profile.eye(centre + towards * (radius * 2.0F));
    profile.up(glm::vec3(0.0F, 1.0F, 0.0F));
    profile.lookat(centre);
    camera.createProjection();
    camera.createView();

    const glm::mat4 expected = camera.projection() * camera.view();
    const glm::mat4 light = lighting::shadowLight(towards, {.centre = centre, .radius = radius}, hand);
    for (int column = 0; column < 4; column++) {
        for (int row = 0; row < 4; row++) {
            BOOST_TEST_CONTEXT("[" << column << "][" << row << "]") {
                BOOST_CHECK_SMALL(light[column][row] - expected[column][row], 1e-5F);
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
    return glm::normalize(glm::vec3(-0.45F, 0.62F, 0.64F));
}

/** Where a camera is, the point it looks at, and how far it sees. **/
struct Looking {
    glm::vec3 eye{0.0F};
    glm::vec3 at{0.0F};
    float farPlane = 100.0F;
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
    profile.fov(60.0F);
    profile.pixelAspect(1.0F);
    profile.clipping(glm::vec2(0.5F, farPlane));
    profile.eye(eye);
    profile.up(glm::vec3(0.0F, 1.0F, 0.0F));
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
    for (const float x : {-1.0F, 1.0F}) {
        for (const float y : {-1.0F, 1.0F}) {
            const glm::vec4 nearClip = inverse * glm::vec4(x, y, 0.0F, 1.0F);
            const glm::vec4 farClip = inverse * glm::vec4(x, y, 1.0F, 1.0F);
            const glm::vec3 nearCorner = glm::vec3(nearClip) / nearClip.w;
            const glm::vec3 farCorner = glm::vec3(farClip) / farClip.w;
            const glm::vec3 edge = farCorner - nearCorner;
            corners.push_back(nearCorner);
            corners.push_back(glm::length(edge) > distance ? nearCorner + glm::normalize(edge) * distance : farCorner);
        }
    }
    return corners;
}

BOOST_AUTO_TEST_SUITE(shadow_test)

/**
 * The eye stands two radii out and the far plane four beyond it, so the centre is in the middle
 * of the map at half depth, whatever the light's direction.
 **/
BOOST_AUTO_TEST_CASE(the_centre_is_the_middle_of_the_map_at_half_depth) {
    const glm::vec3 centre(3.0F, -1.0F, 7.0F);
    const glm::mat4 light = lighting::shadowLight(glm::vec3(-0.45F, 0.62F, 0.64F), {.centre = centre, .radius = 5.0F});

    const glm::vec3 at = project(light, centre);
    BOOST_CHECK_SMALL(at.x, 1e-5F);
    BOOST_CHECK_SMALL(at.y, 1e-5F);
    BOOST_CHECK_CLOSE(at.z, 0.5F, 1e-3F);
}

/**
 * The box reaches a radius either side, so a point that far along the light's right is at the
 * map's edge. A point that far towards or away from the light is a quarter of the depth nearer
 * or further.
 **/
BOOST_AUTO_TEST_CASE(a_radius_out_is_the_edge_of_the_map) {
    const glm::vec3 towards = glm::normalize(glm::vec3(-0.45F, 0.62F, 0.64F));
    const glm::vec3 centre(3.0F, -1.0F, 7.0F);
    const float radius = 5.0F;
    const glm::mat4 light = lighting::shadowLight(towards, {.centre = centre, .radius = radius});

    // right is up crossed into the view, as type::camera::Profile::lookat takes it
    const glm::vec3 right = glm::normalize(glm::cross(glm::vec3(0.0F, 1.0F, 0.0F), -towards));
    BOOST_CHECK_CLOSE(project(light, centre + right * radius).x, 1.0F, 1e-3F);
    BOOST_CHECK_CLOSE(project(light, centre - right * radius).x, -1.0F, 1e-3F);
    BOOST_CHECK_CLOSE(project(light, centre + towards * radius).z, 0.25F, 1e-3F);
    BOOST_CHECK_CLOSE(project(light, centre - towards * radius).z, 0.75F, 1e-3F);
}

/**
 * The light's matrix is the one a type::camera camera builds at the same eye, looking at the
 * same point through the same box. The shadow pass can then cull with the cel pass's winding. A
 * matrix built any other way can reverse the winding of every face on screen.
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
 * A light straight overhead looks straight down, along the usual up, so crossing up with the
 * view gives no right. The matrix takes z as up instead, and its values stay finite, not NaN.
 **/
BOOST_AUTO_TEST_CASE(a_light_overhead_takes_z_as_up) {
    const glm::vec3 centre(1.0F, 0.0F, 2.0F);
    const glm::mat4 light = lighting::shadowLight(glm::vec3(0.0F, 3.0F, 0.0F), {.centre = centre, .radius = 2.0F});

    for (int column = 0; column < 4; column++) {
        for (int row = 0; row < 4; row++) {
            BOOST_CHECK(std::isfinite(light[column][row]));
        }
    }
    const glm::vec3 at = project(light, centre);
    BOOST_CHECK_SMALL(at.x, 1e-5F);
    BOOST_CHECK_SMALL(at.y, 1e-5F);
    BOOST_CHECK_CLOSE(at.z, 0.5F, 1e-3F);
    // z up crossed into a view along -y is +x, so the world's x is the map's right
    BOOST_CHECK_CLOSE(project(light, centre + glm::vec3(2.0F, 0.0F, 0.0F)).x, 1.0F, 1e-3F);
}

/**
 * The centre is the mean of the casters and the radius the farthest of them plus the margin.
 * The ground casts nothing, and an extra point to cover counts as a caster.
 **/
BOOST_AUTO_TEST_CASE(the_fit_covers_the_casters_and_not_the_ground) {
    entt::registry registry;
    caster(&registry, glm::vec3(0.0F, 0.0F, 0.0F), true);
    caster(&registry, glm::vec3(4.0F, 0.0F, 0.0F), true);
    caster(&registry, glm::vec3(2.0F, 0.0F, 6.0F), true);
    caster(&registry, glm::vec3(100.0F, 0.0F, 100.0F), false);

    const std::optional<lighting::ShadowBounds> fitted = lighting::fitShadow(registry, {}, {.margin = 3.0F});
    const std::vector<glm::vec3> cover{glm::vec3(2.0F, 0.0F, -10.0F)};
    const std::optional<lighting::ShadowBounds> covered = lighting::fitShadow(registry, cover, {.margin = 3.0F});
    if (!fitted || !covered) {
        BOOST_ERROR("three casters gave no fit");
        return;
    }

    BOOST_CHECK_CLOSE(fitted->centre.x, 2.0F, 1e-3F);
    BOOST_CHECK_SMALL(fitted->centre.y, 1e-5F);
    BOOST_CHECK_CLOSE(fitted->centre.z, 2.0F, 1e-3F);
    // (2, 0, 6) is four from the centre, and the other two are sqrt(8)
    BOOST_CHECK_CLOSE(fitted->radius, 7.0F, 1e-3F);
    BOOST_CHECK_CLOSE(covered->centre.z, -1.0F, 1e-3F);
}

/**
 * One caster with no margin has no spread. The radius is held at the minimum rather than zero,
 * so the light matrix built from it is finite.
 **/
BOOST_AUTO_TEST_CASE(one_caster_with_no_margin_fits_a_finite_light) {
    entt::registry registry;
    caster(&registry, glm::vec3(1.0F, 2.0F, 3.0F), true);

    const std::optional<lighting::ShadowBounds> fitted = lighting::fitShadow(registry, {}, {.margin = 0.0F});
    const std::optional<lighting::ShadowBounds> shrunk = lighting::fitShadow(registry, {}, {.margin = -1.0F});
    if (!fitted || !shrunk) {
        BOOST_ERROR("one caster gave no fit");
        return;
    }
    BOOST_CHECK_EQUAL(fitted->radius, lighting::minimumShadowRadius);
    BOOST_CHECK_EQUAL(shrunk->radius, lighting::minimumShadowRadius);

    const glm::mat4 light = lighting::shadowLight(glm::vec3(-0.45F, 0.62F, 0.64F), *fitted);
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
    caster(&registry, glm::vec3(1.0F), false);
    const std::vector<glm::vec3> cover{glm::vec3(0.0F)};

    BOOST_CHECK(!lighting::fitShadow(registry, cover, {.margin = 3.0F}));
    BOOST_CHECK(!lighting::fitShadow(entt::registry(), {}, {.margin = 3.0F}));
}

/**
 * A caster is fitted where it is drawn, alpha of the way from its previous step, and at an
 * alpha of one where its current step puts it.
 **/
BOOST_AUTO_TEST_CASE(the_fit_reads_the_drawn_position) {
    entt::registry registry;
    caster(&registry, glm::vec3(0.0F, 0.0F, 0.0F), true);
    v3d::ecs::snapshot<Transform>(registry);
    for (auto [entity, transform] : registry.view<Transform>().each()) {
        transform.position = glm::vec3(10.0F, 0.0F, 0.0F);
    }

    const std::optional<lighting::ShadowBounds> halfway = lighting::fitShadow(registry, {}, {.margin = 1.0F, .alpha = 0.5F});
    const std::optional<lighting::ShadowBounds> current = lighting::fitShadow(registry, {}, {.margin = 1.0F});
    if (!halfway || !current) {
        BOOST_ERROR("a moving caster gave no fit");
        return;
    }
    BOOST_CHECK_CLOSE(halfway->centre.x, 5.0F, 1e-3F);
    BOOST_CHECK_CLOSE(current->centre.x, 10.0F, 1e-3F);
}

/**
 * The view fit encloses the camera's view volume out to the distance, and stops at the far
 * plane when that is nearer.
 **/
BOOST_AUTO_TEST_CASE(the_view_fit_encloses_what_the_camera_sees) {
    const glm::mat4 camera = perspective({.eye = glm::vec3(3.0F, 10.0F, 12.0F), .at = glm::vec3(0.0F), .farPlane = 200.0F});
    const std::optional<lighting::ShadowBounds> fitted = lighting::fitShadowToView(camera, 30.0F, towards(), 1024);
    if (!fitted) {
        BOOST_ERROR("a view gave no fit");
        return;
    }

    for (const glm::vec3& corner : viewCorners(camera, 30.0F)) {
        BOOST_CHECK_LE(glm::length(corner - fitted->centre), fitted->radius);
    }

    // a far plane nearer than the distance bounds the volume instead
    const glm::mat4 shallow = perspective({.eye = glm::vec3(3.0F, 10.0F, 12.0F), .at = glm::vec3(0.0F), .farPlane = 20.0F});
    const std::optional<lighting::ShadowBounds> cut = lighting::fitShadowToView(shallow, 30.0F, towards(), 1024);
    const std::optional<lighting::ShadowBounds> farther = lighting::fitShadowToView(camera, 300.0F, towards(), 1024);
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
    const std::optional<lighting::ShadowBounds> here = lighting::fitShadowToView(perspective({.eye = glm::vec3(3.0F, 10.0F, 12.0F), .at = glm::vec3(0.0F), .farPlane = 200.0F}),
        30.0F, towards(), 1024);
    const std::optional<lighting::ShadowBounds> moved = lighting::fitShadowToView(
        perspective({.eye = glm::vec3(-40.0F, 13.0F, 7.0F), .at = glm::vec3(-20.0F, 0.0F, 31.0F), .farPlane = 200.0F}), 30.0F, towards(), 1024);
    if (!here || !moved) {
        BOOST_ERROR("a view gave no fit");
        return;
    }
    BOOST_CHECK_CLOSE(here->radius, moved->radius, 1e-2F);
}

/**
 * A point fixed in the world falls at the same place within its texel however far the camera
 * slides, so a shadow's edge does not crawl. The light may be built in either hand.
 **/
BOOST_AUTO_TEST_CASE(the_view_fit_keeps_the_world_on_its_texels) {
    const uint32_t texels = 1024;
    const glm::vec3 point(1.3F, 0.0F, -2.7F);
    for (const v3d::type::camera::Profile::Hand hand :
        {v3d::type::camera::Profile::Hand::UpCrossDirection, v3d::type::camera::Profile::Hand::DirectionCrossUp}) {
        std::vector<glm::vec2> within;
        for (const float slide : {0.0F, 0.013F, 0.37F, 2.9F}) {
            const glm::vec3 offset(slide, 0.0F, slide * 0.5F);
            const glm::mat4 camera = perspective({.eye = glm::vec3(3.0F, 10.0F, 12.0F) + offset, .at = offset, .farPlane = 200.0F});
            const std::optional<lighting::ShadowBounds> fitted = lighting::fitShadowToView(camera, 30.0F, towards(), texels);
            if (!fitted) {
                BOOST_ERROR("a view gave no fit");
                return;
            }
            const glm::vec3 clip = project(lighting::shadowLight(towards(), *fitted, hand), point);
            const glm::vec2 texel = (glm::vec2(clip) + 1.0F) * 0.5F * static_cast<float>(texels);
            within.emplace_back(texel.x - std::floor(texel.x), texel.y - std::floor(texel.y));
        }
        for (const glm::vec2& fraction : within) {
            BOOST_CHECK_SMALL(fraction.x - within.front().x, 2e-2F);
            BOOST_CHECK_SMALL(fraction.y - within.front().y, 2e-2F);
        }
    }
}

/**
 * A distance, a direction, a map width or a matrix that cannot be fitted gives nothing.
 **/
BOOST_AUTO_TEST_CASE(the_view_fit_refuses_what_it_cannot_fit) {
    const glm::mat4 camera = perspective({.eye = glm::vec3(3.0F, 10.0F, 12.0F), .at = glm::vec3(0.0F), .farPlane = 200.0F});
    const float nan = std::numeric_limits<float>::quiet_NaN();

    BOOST_CHECK(!lighting::fitShadowToView(camera, 0.0F, towards(), 1024));
    BOOST_CHECK(!lighting::fitShadowToView(camera, nan, towards(), 1024));
    BOOST_CHECK(!lighting::fitShadowToView(camera, std::numeric_limits<float>::infinity(), towards(), 1024));
    BOOST_CHECK(!lighting::fitShadowToView(camera, 30.0F, glm::vec3(0.0F), 1024));
    BOOST_CHECK(!lighting::fitShadowToView(camera, 30.0F, glm::vec3(nan, 1.0F, 0.0F), 1024));
    BOOST_CHECK(!lighting::fitShadowToView(camera, 30.0F, towards(), 3));
    BOOST_CHECK(!lighting::fitShadowToView(glm::mat4(0.0F), 30.0F, towards(), 1024));
}

/**
 * The practical split scheme at its two ends: a lambda of zero gives cascades of one length,
 * and one gives cascades of one ratio of far to near, both measured from the eye. Between them
 * is a blend, and every split ends at the distance.
 **/
BOOST_AUTO_TEST_CASE(the_view_splits_from_uniform_to_logarithmic) {
    const std::vector<float> uniform = lighting::splitView(0.5F, 90.0F, 3, 0.0F);
    BOOST_REQUIRE_EQUAL(uniform.size(), 3U);
    BOOST_CHECK_CLOSE(uniform[0], 30.0F, 1e-3F);
    BOOST_CHECK_CLOSE(uniform[1], 60.0F, 1e-3F);
    BOOST_CHECK_EQUAL(uniform[2], 90.0F);

    // from the eye, the splits are 0.5 * 182^(i / 3), which keeps far over near the same
    const std::vector<float> logarithmic = lighting::splitView(0.5F, 90.5F, 3, 1.0F);
    BOOST_REQUIRE_EQUAL(logarithmic.size(), 3U);
    const float ratio = std::cbrt(182.0F);
    BOOST_CHECK_CLOSE(logarithmic[0] + 0.5F, 0.5F * ratio, 1e-2F);
    BOOST_CHECK_CLOSE(logarithmic[1] + 0.5F, 0.5F * ratio * ratio, 1e-2F);
    BOOST_CHECK_EQUAL(logarithmic[2], 90.5F);

    const std::vector<float> blended = lighting::splitView(0.5F, 90.5F, 3, 0.5F);
    BOOST_REQUIRE_EQUAL(blended.size(), 3U);
    BOOST_CHECK_CLOSE(blended[0], 0.5F * (logarithmic[0] + (90.5F / 3.0F)), 1e-2F);
    BOOST_CHECK_LT(blended[0], blended[1]);

    // a lambda past either end is held there, and one cascade is the whole distance
    BOOST_CHECK(lighting::splitView(0.5F, 90.0F, 3, -2.0F) == uniform);
    const std::vector<float> one = lighting::splitView(0.5F, 90.0F, 1, 0.75F);
    BOOST_REQUIRE_EQUAL(one.size(), 1U);
    BOOST_CHECK_EQUAL(one[0], 90.0F);
}

/**
 * A near plane, a distance, a count or a lambda that cannot be split gives no splits.
 **/
BOOST_AUTO_TEST_CASE(the_view_split_refuses_what_it_cannot_split) {
    const float nan = std::numeric_limits<float>::quiet_NaN();
    BOOST_CHECK(lighting::splitView(0.0F, 90.0F, 3, 0.5F).empty());
    BOOST_CHECK(lighting::splitView(0.5F, 0.0F, 3, 0.5F).empty());
    BOOST_CHECK(lighting::splitView(0.5F, std::numeric_limits<float>::infinity(), 3, 0.5F).empty());
    BOOST_CHECK(lighting::splitView(0.5F, 90.0F, 0, 0.5F).empty());
    BOOST_CHECK(lighting::splitView(0.5F, 90.0F, lighting::maximumCascades + 1, 0.5F).empty());
    BOOST_CHECK(lighting::splitView(0.5F, 90.0F, 3, nan).empty());
}

/** A camera to fit cascades to, described for a check that does not use the fit. **/
struct Viewed final {
    glm::mat4 viewProjection;
    glm::vec3 eye;
    glm::vec3 forward;
    float near = 0.5F;
    /** whether every ray runs along forward, rather than out from the eye **/
    bool orthographic = false;
};

/** A point of a view, and its distance from the near plane. **/
struct Sampled final {
    glm::vec3 point;
    float distance = 0.0F;
};

/**
 * Whether a cascade reaches a point's distance, and check that the point lies in the sphere of
 * every cascade that does. Cascade i reaches from split i - 1, less the blend, to split i.
 **/
bool covers(const lighting::Cascades& fitted, const std::vector<float>& splits, const Sampled& sampled) {
    bool covered = false;
    for (std::size_t i = 0; i < fitted.count; i++) {
        const float from = i == 0 ? 0.0F : splits[i - 1] * (1.0F - fitted.blend);
        if (sampled.distance < from - 1e-3F || sampled.distance > splits[i] + 1e-3F) {
            continue;
        }
        covered = true;
        BOOST_CHECK_LE(glm::length(sampled.point - fitted.bounds[i].centre), fitted.bounds[i].radius * 1.0001F);
    }
    return covered;
}

/**
 * Check that every point of a view out to the last split lies in a cascade that reaches it, and
 * so in that cascade's sphere. A cascade reaches from the end of the one before it, less the
 * blend, to its own split, so the two share a band. The points lie along rays through the edges
 * and the inside of the picture, stepped evenly by distance, so each shared band is sampled
 * finely. The distance is measured from the camera, not from what the fit returns, and the
 * fit's plane gives the same distance.
 **/
void checkCovered(const Viewed& viewed, const std::vector<float>& splits) {
    const float blend = 0.1F;
    const std::optional<lighting::Cascades> fitted = lighting::fitCascades(viewed.viewProjection, splits, towards(),
        {.texels = 1024, .blend = blend});
    if (!fitted) {
        BOOST_ERROR("a view gave no cascades");
        return;
    }
    BOOST_REQUIRE_EQUAL(fitted->count, splits.size());
    for (std::size_t i = 0; i < splits.size(); i++) {
        BOOST_CHECK_EQUAL(fitted->far[i], splits[i]);
    }

    const glm::mat4 inverse = glm::inverse(viewed.viewProjection);
    const int steps = 400;
    for (int x = -4; x <= 4; x++) {
        for (int y = -4; y <= 4; y++) {
            const glm::vec4 clip = inverse * glm::vec4(static_cast<float>(x) / 4.0F, static_cast<float>(y) / 4.0F, 0.0F, 1.0F);
            const glm::vec3 onNear = glm::vec3(clip) / clip.w;
            const glm::vec3 ray = viewed.orthographic ? viewed.forward : glm::normalize(onNear - viewed.eye);
            for (int step = 0; step <= steps; step++) {
                const float distance = splits.back() * static_cast<float>(step) / static_cast<float>(steps);
                const glm::vec3 point = onNear + ray * (distance / glm::dot(ray, viewed.forward));
                BOOST_CHECK_SMALL(glm::dot(glm::vec3(fitted->plane), point) + fitted->plane.w - distance, 1e-2F);
                BOOST_CHECK(covers(*fitted, splits, {.point = point, .distance = distance}));
            }
        }
    }
    // a nearer cascade covers less, so its texels are smaller
    for (std::size_t i = 1; i < fitted->count; i++) {
        BOOST_CHECK_LT(fitted->bounds[i - 1].radius, fitted->bounds[i].radius);
    }
}

/**
 * The cascades cover a perspective view without gaps, and a narrow orthographic one too. A
 * narrow view's slices are long and thin, so each sphere hugs its slice, and only a cascade
 * that starts inside the one before it covers the band the two share.
 **/
BOOST_AUTO_TEST_CASE(the_cascades_cover_the_view_without_gaps) {
    const glm::vec3 eye(3.0F, 10.0F, 12.0F);
    const glm::vec3 at(0.0F);
    checkCovered({.viewProjection = perspective({.eye = eye, .at = at, .farPlane = 200.0F}), .eye = eye,
        .forward = glm::normalize(at - eye)}, {8.0F, 25.0F, 70.0F});

    // one unit wide and high, out to 200, with depth from 0 at the near plane to 1 at the far
    const glm::mat4 narrow = glm::orthoRH_ZO(-0.5F, 0.5F, -0.5F, 0.5F, 0.5F, 200.0F) *
        glm::lookAtRH(eye, at, glm::vec3(0.0F, 1.0F, 0.0F));
    checkCovered({.viewProjection = narrow, .eye = eye, .forward = glm::normalize(at - eye), .orthographic = true},
        {8.0F, 25.0F, 70.0F});
}

/**
 * Each cascade keeps its radius as the camera moves and turns, so its texels stay the same
 * size. As the camera slides, a point fixed in the world stays at the same place within its
 * texel.
 **/
BOOST_AUTO_TEST_CASE(the_cascades_hold_still_as_the_camera_moves) {
    const std::vector<float> splits = {8.0F, 25.0F, 70.0F};
    const std::optional<lighting::Cascades> here = lighting::fitCascades(
        perspective({.eye = glm::vec3(3.0F, 10.0F, 12.0F), .at = glm::vec3(0.0F), .farPlane = 200.0F}), splits, towards());
    const std::optional<lighting::Cascades> turned = lighting::fitCascades(
        perspective({.eye = glm::vec3(-40.0F, 13.0F, 7.0F), .at = glm::vec3(-20.0F, 0.0F, 31.0F), .farPlane = 200.0F}), splits, towards());
    if (!here || !turned) {
        BOOST_ERROR("a view gave no cascades");
        return;
    }
    for (std::size_t i = 0; i < splits.size(); i++) {
        BOOST_CHECK_CLOSE(here->bounds[i].radius, turned->bounds[i].radius, 1e-2F);
    }

    const uint32_t texels = 1024;
    const glm::vec3 point(1.3F, 0.0F, -2.7F);
    for (const std::size_t cascade : {0U, 1U}) {
        std::vector<glm::vec2> within;
        for (const float slide : {0.0F, 0.013F, 0.37F, 2.9F}) {
            const glm::vec3 offset(slide, 0.0F, slide * 0.5F);
            const std::optional<lighting::Cascades> slid = lighting::fitCascades(
                perspective({.eye = glm::vec3(3.0F, 10.0F, 12.0F) + offset, .at = offset, .farPlane = 200.0F}), splits, towards(),
                {.texels = texels});
            if (!slid) {
                BOOST_ERROR("a view gave no cascades");
                return;
            }
            const glm::vec3 clip = project(slid->lights[cascade], point);
            const glm::vec2 texel = (glm::vec2(clip) + 1.0F) * 0.5F * static_cast<float>(texels);
            within.emplace_back(texel.x - std::floor(texel.x), texel.y - std::floor(texel.y));
        }
        for (const glm::vec2& fraction : within) {
            BOOST_TEST_CONTEXT("cascade " << cascade) {
                BOOST_CHECK_SMALL(fraction.x - within.front().x, 2e-2F);
                BOOST_CHECK_SMALL(fraction.y - within.front().y, 2e-2F);
            }
        }
    }
}

/**
 * Splits that do not increase, too many or none, a direction, a width or a matrix that cannot
 * be fitted give no cascades.
 **/
BOOST_AUTO_TEST_CASE(the_cascade_fit_refuses_what_it_cannot_fit) {
    const glm::mat4 camera = perspective({.eye = glm::vec3(3.0F, 10.0F, 12.0F), .at = glm::vec3(0.0F), .farPlane = 200.0F});
    const float nan = std::numeric_limits<float>::quiet_NaN();
    const std::vector<float> splits = {8.0F, 25.0F};
    BOOST_CHECK(!lighting::fitCascades(camera, std::vector<float>{}, towards()));
    BOOST_CHECK(!lighting::fitCascades(camera, std::vector<float>{8.0F, 8.0F}, towards()));
    BOOST_CHECK(!lighting::fitCascades(camera, std::vector<float>{0.0F, 8.0F}, towards()));
    BOOST_CHECK(!lighting::fitCascades(camera, std::vector<float>{8.0F, nan}, towards()));
    BOOST_CHECK(!lighting::fitCascades(camera, std::vector<float>{1.0F, 2.0F, 3.0F, 4.0F, 5.0F}, towards()));
    BOOST_CHECK(!lighting::fitCascades(camera, splits, glm::vec3(0.0F)));
    BOOST_CHECK(!lighting::fitCascades(camera, splits, towards(), {.texels = 3}));
    BOOST_CHECK(!lighting::fitCascades(glm::mat4(0.0F), splits, towards()));
    BOOST_CHECK(lighting::fitCascades(camera, splits, towards()).has_value());
}

/**
 * A caster throws a shadow into a cascade while a sphere around it meets the box the cascade is
 * drawn through. The box reaches a radius either side across the light, and two radii towards
 * and away from it. Just past any face of the box the caster throws no shadow.
 **/
BOOST_AUTO_TEST_CASE(a_caster_casts_into_a_cascade_while_it_meets_its_box) {
    const lighting::ShadowBounds cascade{.centre = glm::vec3(5.0F, 0.0F, 0.0F), .radius = 10.0F};
    const glm::vec3 overhead(0.0F, 1.0F, 0.0F);
    const auto at = [&](const glm::vec3& offset) {
        return lighting::casts(cascade, overhead, {.centre = cascade.centre + offset, .radius = 1.0F});
    };
    BOOST_TEST(at(glm::vec3(0.0F)));
    // across the light, a radius and the caster's own either side
    BOOST_TEST(at(glm::vec3(10.9F, 0.0F, 0.0F)));
    BOOST_TEST(!at(glm::vec3(11.1F, 0.0F, 0.0F)));
    BOOST_TEST(at(glm::vec3(0.0F, 0.0F, -10.9F)));
    BOOST_TEST(!at(glm::vec3(0.0F, 0.0F, -11.1F)));
    // along it, two radii towards the light and two away
    BOOST_TEST(at(glm::vec3(0.0F, 20.9F, 0.0F)));
    BOOST_TEST(!at(glm::vec3(0.0F, 21.1F, 0.0F)));
    BOOST_TEST(at(glm::vec3(0.0F, -20.9F, 0.0F)));
    BOOST_TEST(!at(glm::vec3(0.0F, -21.1F, 0.0F)));
}

/**
 * The cascades go into the scene uniform as the lit shaders read them: a matrix, a far end and a
 * radius each, the near plane, the count and the blend. Packed from one matrix, the one cascade
 * reaches every distance.
 **/
BOOST_AUTO_TEST_CASE(the_cascades_pack_into_the_scene) {
    const glm::mat4 camera = perspective({.eye = glm::vec3(3.0F, 10.0F, 12.0F), .at = glm::vec3(0.0F), .farPlane = 200.0F});
    const std::vector<float> splits = {8.0F, 25.0F, 70.0F};
    const std::optional<lighting::Cascades> fitted = lighting::fitCascades(camera, splits, towards(), {.blend = 0.2F});
    if (!fitted) {
        BOOST_ERROR("a view gave no cascades");
        return;
    }
    const lighting::SceneUniforms packed = lighting::pack(lighting::Settings(), *fitted, 1.0F / 1024.0F);
    for (std::size_t i = 0; i < splits.size(); i++) {
        BOOST_CHECK(packed.lightViewProjection[i] == fitted->lights[i]);
        BOOST_CHECK_EQUAL(packed.far[static_cast<int>(i)], splits[i]);
        BOOST_CHECK_EQUAL(packed.radii[static_cast<int>(i)], fitted->bounds[i].radius);
    }
    BOOST_CHECK(packed.lightViewProjection[3] == glm::mat4(1.0F));
    BOOST_CHECK(packed.plane == fitted->plane);
    BOOST_CHECK_EQUAL(packed.cascades.x, 3.0F);
    BOOST_CHECK_CLOSE(packed.cascades.y, 0.2F, 1e-4F);

    const lighting::SceneUniforms one = lighting::pack(lighting::Settings(), fitted->lights[1], 1.0F / 1024.0F);
    BOOST_CHECK(one.lightViewProjection[0] == fitted->lights[1]);
    BOOST_CHECK_EQUAL(one.cascades.x, 1.0F);
    BOOST_CHECK_EQUAL(one.cascades.y, 0.0F);
    BOOST_CHECK_EQUAL(one.far.x, std::numeric_limits<float>::max());
    BOOST_CHECK(one.plane == glm::vec4(0.0F));
    BOOST_CHECK_EQUAL(one.radii.x, 1.0F);
}

BOOST_AUTO_TEST_SUITE_END()

};  // namespace
