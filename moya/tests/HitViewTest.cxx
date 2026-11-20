/*
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 */

#include <moya/libmoya/camera/Camera.h>
#include <moya/libmoya/device/CpuDevice.h>
#include <moya/libmoya/film/Snapshot.h>
#include <moya/libmoya/geometry/Flat.h>
#include <moya/libmoya/geometry/World.h>
#include <moya/libmoya/kernel/camera/Camera.h>
#include <moya/libmoya/kernel/geometry/BruteForce.h>
#include <moya/libmoya/kernel/geometry/Hit.h>
#include <moya/libmoya/kernel/geometry/Ray.h>
#include <moya/libmoya/kernel/geometry/Traversal.h>
#include <moya/libmoya/scene/Handle.h>
#include <moya/libmoya/scene/Kinds.h>
#include <moya/libmoya/scene/Scene.h>
#include <moya/libmoya/scene/Value.h>
#include <moya/libmoya/view/Hits.h>

#include <algorithm>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <string>
#include <vector>

#include <boost/shared_ptr.hpp>
#include <boost/test/unit_test.hpp>

#include <glm/ext/matrix_transform.hpp>
#include <glm/geometric.hpp>
#include <glm/mat4x4.hpp>
#include <glm/trigonometric.hpp>
#include <glm/vec2.hpp>
#include <glm/vec3.hpp>

#include "Tracing.h"

namespace v3d::moya::tracing {
namespace {

namespace scene = v3d::moya::scene;
namespace kernel = v3d::moya::kernel;
namespace geometry = v3d::moya::geometry;
namespace view = v3d::moya::view;

/** A scene's options and camera: a picture of a size, through a perspective camera. **/
scene::Handle framed(scene::Scene & scene, std::int32_t width, std::int32_t height, const glm::mat4 & toWorld) {
    const scene::Handle options = scene.create("options", "options");
    const scene::Handle camera = scene.create("camera", "camera");
    scene.set(options, "resolution", scene::Value::integers({ width, height }));
    scene.set(options, "camera", scene::Value::handle(camera));
    scene.set(camera, "projection", scene::Value::string("perspective"));
    scene.set(camera, "transform", scene::Value::matrix(toWorld));
    return camera;
}

/** A mesh of one face, and an instance of it at a transformation in a group or the world. **/
scene::Handle placed(scene::Scene & scene, const std::string & name, const std::vector<glm::vec3> & points,
    const scene::Handle & group = {}, const glm::mat4 & transform = glm::mat4(1.0F)) {
    const scene::Handle mesh = scene.create("mesh", name);
    std::vector<std::int32_t> indices;
    indices.reserve(points.size());
    for (std::size_t i = 0; i < points.size(); ++i) {
        indices.push_back(static_cast<std::int32_t>(i));
    }
    scene.set(mesh, "P", scene::Value::points(points));
    scene.set(mesh, "vertices", scene::Value::integers({ static_cast<std::int32_t>(points.size()) }));
    scene.set(mesh, "indices", scene::Value::integers(indices));
    const scene::Handle instance = scene.create("instance", name);
    scene.set(instance, "prototype", scene::Value::handle(mesh));
    if (group.valid()) {
        scene.set(instance, "group", scene::Value::handle(group));
    }
    scene.set(instance, "transform", scene::Value::matrix(transform));
    return instance;
}

// a triangle whose corners project onto three corners of the screen covers the pixels on its side
// of the diagonal, at its depth, facing the way its winding gives
BOOST_AUTO_TEST_CASE(moya_hitview_a_triangle_covers_the_pixels_its_projection_covers_test) {
    constexpr std::int32_t kSide = 32;
    scene::Scene scene(scene::kinds());
    framed(scene, kSide, kSide, glm::mat4(1.0F));
    const scene::Handle instance = placed(scene, "triangle", { { -2.0F, -2.0F, 2.0F }, { 2.0F, -2.0F, 2.0F }, { -2.0F, 2.0F, 2.0F } });
    scene.commit();
    scene.apply();

    v3d::moya::device::CpuDevice device;
    const v3d::moya::film::Snapshot snapshot = view::hits(device, scene, {});
    const std::uint32_t depth = snapshot.offset(view::kDepth);
    const std::uint32_t normal = snapshot.offset(view::kNormal);
    const std::uint32_t coverage = snapshot.offset(view::kCoverage);
    const std::uint32_t index = snapshot.offset(view::kInstance);
    const std::uint32_t face = snapshot.offset(view::kFace);
    int checked = 0;
    for (std::uint32_t y = 0; y < kSide; ++y) {
        for (std::uint32_t x = 0; x < kSide; ++x) {
            // the screen point of the pixel's centre lies below the diagonal x + y = 0 when the
            // column is left of the row. A centre on the diagonal is on the triangle's edge
            if (x == y) {
                continue;
            }
            ++checked;
            const bool inside = x < y;
            BOOST_TEST((snapshot.at(x, y, coverage) == 1.0F) == inside, "pixel " << x << ", " << y);
            if (inside) {
                BOOST_TEST(std::fabs(snapshot.at(x, y, depth) - 2.0F) < 1.0e-5F);
                BOOST_TEST(snapshot.at(x, y, normal + 2) == 1.0F);
                BOOST_TEST(snapshot.at(x, y, index) == static_cast<float>(instance.index));
                BOOST_TEST(snapshot.at(x, y, face) == 0.0F);
            } else {
                BOOST_TEST(std::isinf(snapshot.at(x, y, depth)));
                BOOST_TEST(snapshot.at(x, y, index) == -1.0F);
                BOOST_TEST(snapshot.at(x, y, face) == -1.0F);
            }
        }
    }
    BOOST_TEST(checked == (kSide * kSide) - kSide);
}

// a camera at x = 10, turned to look down +x, sees a wall across x = 14 at a depth of four in
// every pixel. Depth runs along the view, not along the ray
BOOST_AUTO_TEST_CASE(moya_hitview_a_pixels_depth_is_its_hits_distance_along_the_view_test) {
    constexpr std::int32_t kSide = 16;
    scene::Scene scene(scene::kinds());
    const glm::mat4 toWorld = glm::rotate(glm::translate(glm::mat4(1.0F), glm::vec3 { 10.0F, 0.0F, 0.0F }), glm::radians(90.0F),
        glm::vec3 { 0.0F, 1.0F, 0.0F });
    framed(scene, kSide, kSide, toWorld);
    placed(scene, "wall", { { 14.0F, -50.0F, -50.0F }, { 14.0F, 50.0F, -50.0F }, { 14.0F, 50.0F, 50.0F }, { 14.0F, -50.0F, 50.0F } });
    scene.commit();
    scene.apply();

    v3d::moya::device::CpuDevice device;
    const v3d::moya::film::Snapshot snapshot = view::hits(device, scene, {});
    const std::uint32_t depth = snapshot.offset(view::kDepth);
    const std::uint32_t normal = snapshot.offset(view::kNormal);
    for (std::uint32_t y = 0; y < kSide; ++y) {
        for (std::uint32_t x = 0; x < kSide; ++x) {
            BOOST_TEST(std::fabs(snapshot.at(x, y, depth) - 4.0F) < 1.0e-4F, "pixel " << x << ", " << y);
            BOOST_TEST(std::fabs(std::fabs(snapshot.at(x, y, normal)) - 1.0F) < 1.0e-6F);
        }
    }
}

BOOST_AUTO_TEST_CASE(moya_hitview_an_orthographic_cameras_rays_are_parallel_test) {
    kernel::camera::Camera camera;
    camera.resolution = glm::vec2 { 64.0F, 48.0F };
    camera.near = 1.0F;
    camera.far = 100.0F;
    const kernel::geometry::Ray first = camera.ray({ 0.5F, 0.5F }, { 0.5F, 0.5F }, 0.0F);
    const kernel::geometry::Ray last = camera.ray({ 63.5F, 47.5F }, { 0.5F, 0.5F }, 0.0F);
    BOOST_TEST((first.direction == last.direction));
    BOOST_TEST((first.origin != last.origin));
    BOOST_TEST(first.start == 1.0F);
    BOOST_TEST(first.end == 100.0F);
    // the screen's top left is the first pixel's, and y runs up the screen and down the raster
    BOOST_TEST(first.origin.x < last.origin.x);
    BOOST_TEST(first.origin.y > last.origin.y);
}

BOOST_AUTO_TEST_CASE(moya_hitview_a_ray_drawn_inside_the_shutter_carries_its_time_test) {
    kernel::camera::Camera camera;
    camera.open = 1.0F;
    camera.close = 3.0F;
    BOOST_TEST(camera.ray({ 0.5F, 0.5F }, { 0.5F, 0.5F }, 0.0F).time == 1.0F);
    BOOST_TEST(camera.ray({ 0.5F, 0.5F }, { 0.5F, 0.5F }, 0.25F).time == 1.5F);
}

// rays through one pixel from anywhere on the lens meet at the focal distance, and nowhere
// nearer
BOOST_AUTO_TEST_CASE(moya_hitview_rays_from_the_lens_meet_at_the_focal_distance_test) {
    kernel::camera::Camera camera;
    camera.perspective = true;
    camera.resolution = glm::vec2 { 32.0F, 32.0F };
    camera.lensRadius = 0.25F;
    camera.focalDistance = 6.0F;
    const kernel::geometry::Ray centre = camera.ray({ 7.5F, 20.5F }, { 0.5F, 0.5F }, 0.0F);
    const glm::vec3 focus = centre.at(6.0F);
    for (const glm::vec2 & lens : { glm::vec2 { 0.1F, 0.2F }, glm::vec2 { 0.9F, 0.5F }, glm::vec2 { 0.4F, 0.95F } }) {
        const kernel::geometry::Ray ray = camera.ray({ 7.5F, 20.5F }, lens, 0.0F);
        BOOST_TEST(glm::length(ray.at(6.0F) - focus) < 1.0e-5F);
        BOOST_TEST(glm::length(ray.at(3.0F) - centre.at(3.0F)) > 0.01F);
        BOOST_TEST(glm::length(ray.origin) <= 0.25F + 1.0e-6F);
    }
}

// a frame taller than it is wide spans -1 to 1 across, and the field of view across it
BOOST_AUTO_TEST_CASE(moya_hitview_a_tall_frames_screen_spans_its_width_test) {
    scene::Scene scene(scene::kinds());
    framed(scene, 20, 40, glm::mat4(1.0F));
    scene.commit();
    scene.apply();
    const kernel::camera::Camera camera = v3d::moya::camera::fromScene(scene);
    BOOST_TEST(camera.screen.x == -1.0F);
    BOOST_TEST(camera.screen.y == 1.0F);
    BOOST_TEST(camera.screen.z == -2.0F);
    BOOST_TEST(camera.screen.w == 2.0F);
    BOOST_TEST(std::fabs(camera.tanHalfFov - 1.0F) < 1.0e-6F);
}

// of two triangles across one ray, the nearer is the hit whichever was placed first, and the any
// test finds one
BOOST_AUTO_TEST_CASE(moya_hitview_the_brute_force_traversal_finds_the_nearest_hit_test) {
    scene::Scene scene(scene::kinds());
    const std::vector<glm::vec3> far { { -1.0F, -1.0F, 5.0F }, { 1.0F, -1.0F, 5.0F }, { 0.0F, 1.0F, 5.0F } };
    const std::vector<glm::vec3> near { { -1.0F, -1.0F, 3.0F }, { 1.0F, -1.0F, 3.0F }, { 0.0F, 1.0F, 3.0F } };
    const scene::Handle nearer = placed(scene, "near", near);
    placed(scene, "far", far);
    scene.commit();
    scene.apply();

    v3d::moya::device::CpuDevice device;
    const geometry::World world(device, geometry::flatten(scene, {}));
    const kernel::geometry::BruteForce traversal { .placed = world.view() };
    kernel::geometry::Ray ray;
    ray.direction = glm::vec3 { 0.0F, 0.0F, 1.0F };
    kernel::geometry::EveryHit every;
    const kernel::geometry::Hit hit = traversal.nearest(ray, every);
    BOOST_TEST(hit.found());
    BOOST_TEST(hit.distance == 3.0F);
    BOOST_TEST(hit.depth == 1U);
    BOOST_TEST(hit.instances[0] == nearer.index);
    BOOST_TEST(traversal.any(ray, every));

    ray.end = 2.0F;
    BOOST_TEST(!traversal.nearest(ray, every).found());
    BOOST_TEST(!traversal.any(ray, every));
}

// a group placed twice places its mesh at both of its instances' transformations, each a chain
// of two instances
BOOST_AUTO_TEST_CASE(moya_hitview_a_group_in_a_group_places_its_geometry_at_both_test) {
    scene::Scene scene(scene::kinds());
    const scene::Handle group = scene.create("group", "pair");
    const scene::Handle inner = placed(scene, "piece", { { 0.0F, 0.0F, 0.0F }, { 1.0F, 0.0F, 0.0F }, { 0.0F, 1.0F, 0.0F } }, group,
        glm::translate(glm::mat4(1.0F), glm::vec3 { 0.0F, 0.0F, 1.0F }));
    const scene::Handle left = instanced(scene, "left", group, {}, glm::translate(glm::mat4(1.0F), glm::vec3 { -10.0F, 0.0F, 0.0F }));
    const scene::Handle right = instanced(scene, "right", group, {}, glm::translate(glm::mat4(1.0F), glm::vec3 { 10.0F, 0.0F, 0.0F }));
    scene.commit();
    scene.apply();

    const geometry::Flat flat = geometry::flatten(scene, {});
    BOOST_TEST(flat.reports.empty());
    BOOST_TEST(flat.placements.size() == 2U);
    BOOST_TEST(flat.points.size() == 6U);
    BOOST_TEST(flat.placements[0].depth == 2U);
    BOOST_TEST(flat.placements[0].instances[0] == left.index);
    BOOST_TEST(flat.placements[0].instances[1] == inner.index);
    BOOST_TEST(flat.placements[1].instances[0] == right.index);
    BOOST_TEST((flat.points[0] == glm::vec3(-10.0F, 0.0F, 1.0F)));
    BOOST_TEST((flat.points[3] == glm::vec3(10.0F, 0.0F, 1.0F)));
}

// a group that holds an instance of itself is reported and left out where it does, and what it
// places otherwise is kept
BOOST_AUTO_TEST_CASE(moya_hitview_a_group_that_holds_itself_is_reported_and_left_out_test) {
    scene::Scene scene(scene::kinds());
    const scene::Handle group = scene.create("group", "loop");
    placed(scene, "piece", { { 0.0F, 0.0F, 0.0F }, { 1.0F, 0.0F, 0.0F }, { 0.0F, 1.0F, 0.0F } }, group);
    instanced(scene, "again", group, group, glm::mat4(1.0F));
    instanced(scene, "outer", group, {}, glm::mat4(1.0F));
    scene.commit();
    scene.apply();

    const geometry::Flat flat = geometry::flatten(scene, {});
    BOOST_TEST(reported(flat, "group loop holds itself"));
    BOOST_TEST(flat.reports.size() == 1U);
    BOOST_TEST(flat.placements.size() == 1U);
}

// groups nested one deeper than a hit records are reported and left out, and the chains that fit
// are kept
BOOST_AUTO_TEST_CASE(moya_hitview_a_chain_deeper_than_a_hit_records_is_left_out_test) {
    scene::Scene scene(scene::kinds());
    std::vector<scene::Handle> groups;
    groups.reserve(kernel::geometry::kInstanceDepth);
    for (std::uint32_t level = 0; level < kernel::geometry::kInstanceDepth; ++level) {
        groups.push_back(scene.create("group", "level " + std::to_string(level)));
    }
    // the deepest group holds the mesh, each group holds the one below it, and the world the top
    placed(scene, "piece", { { 0.0F, 0.0F, 0.0F }, { 1.0F, 0.0F, 0.0F }, { 0.0F, 1.0F, 0.0F } }, groups.back());
    for (std::size_t level = 0; level + 1 < groups.size(); ++level) {
        instanced(scene, "into " + std::to_string(level + 1), groups[level + 1], groups[level], glm::mat4(1.0F));
    }
    instanced(scene, "top", groups.front(), {}, glm::mat4(1.0F));
    // the same mesh placed by a chain that fits
    instanced(scene, "short", groups.back(), {}, glm::mat4(1.0F));
    scene.commit();
    scene.apply();

    const geometry::Flat flat = geometry::flatten(scene, {});
    BOOST_TEST(reported(flat, "is nested deeper than"));
    BOOST_TEST(flat.placements.size() == 1U);
    BOOST_TEST(flat.placements[0].depth == 2U);
}

};  // namespace
};  // namespace v3d::moya::tracing
