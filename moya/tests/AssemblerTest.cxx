/*
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 */

#include <moya/libmoya/accel/Assembler.h>
#include <moya/libmoya/device/CpuDevice.h>
#include <moya/libmoya/geometry/Group.h>
#include <moya/libmoya/geometry/Prototypes.h>
#include <moya/libmoya/kernel/geometry/Ray.h>
#include <moya/libmoya/scene/Handle.h>
#include <moya/libmoya/scene/Kinds.h>
#include <moya/libmoya/scene/Scene.h>
#include <moya/libmoya/scene/Value.h>
#include <moya/libmoya/view/Traced.h>
#include <moya/libmoya/view/Tracing.h>

#include <cstddef>
#include <cstdint>
#include <string>
#include <vector>

#include <boost/test/unit_test.hpp>

#include <glm/mat4x4.hpp>
#include <glm/vec3.hpp>

#include "Tracing.h"

namespace v3d::moya::tracing {
namespace {

namespace accel = v3d::moya::accel;
namespace geometry = v3d::moya::geometry;
namespace kernel = v3d::moya::kernel;
namespace scene = v3d::moya::scene;
namespace view = v3d::moya::view;
typedef v3d::moya::device::CpuDevice CpuDevice;
typedef std::vector<std::uint32_t> Indices;

/**
 * A scene of a triangle placed at x = -10 in the world, and another in a group the world places
 * at x = 10. Each rises from z = 0 to z = 1 over the shutter, so each holds times of its own.
 **/
struct Placed {
    scene::Scene scene { scene::kinds() };
    scene::Handle left;
    scene::Handle right;
    scene::Handle group;
    scene::Handle leftInstance;
    scene::Handle rightInstance;
    scene::Handle groupInstance;

    /** One triangle with its right angle on the z axis, at a height. **/
    static std::vector<glm::vec3> piece(float height) {
        return { { 0.0F, 0.0F, height }, { 1.0F, 0.0F, height }, { 0.0F, 1.0F, height } };
    }

    Placed() {
        shutter(scene);
        left = mesh(scene, "left", { 0.0F, 1.0F }, { piece(0.0F), piece(1.0F) });
        right = mesh(scene, "right", { 0.0F, 1.0F }, { piece(0.0F), piece(1.0F) });
        group = scene.create("group", "group");
        leftInstance = instanced(scene, "left", left, {}, moved(-10.0F, 0.0F, 0.0F));
        rightInstance = instanced(scene, "right", right, group, glm::mat4(1.0F));
        groupInstance = instanced(scene, "group", group, {}, moved(10.0F, 0.0F, 0.0F));
        scene.commit();
        scene.apply();
    }
};

/** Rays down -z from z = 50, across x from -12 to 12 at y = 0.25, at two times. **/
std::vector<kernel::geometry::Ray> across() {
    std::vector<kernel::geometry::Ray> out;
    for (const float time : { 0.0F, 0.5F }) {
        for (int step = -48; step <= 48; ++step) {
            kernel::geometry::Ray ray;
            ray.origin = glm::vec3 { static_cast<float>(step) * 0.25F, 0.25F, 50.0F };
            ray.direction = glm::vec3 { 0.0F, 0.0F, -1.0F };
            ray.time = time;
            out.push_back(ray);
        }
    }
    return out;
}

/**
 * Whether the assembler's hierarchies meet every ray where hierarchies built afresh from the
 * scene meet it, naming the same instance and primitive.
 **/
void agrees(CpuDevice & device, const accel::Assembler & assembler, const scene::Scene & scene) {
    const std::vector<kernel::geometry::Ray> rays = across();
    const view::Traced want = view::trace(device, geometry::gather(scene, {}), rays, view::Tracer::Hierarchy);
    const view::Traced got = view::trace(device, assembler.prototypes(), assembler.assembly(), rays);
    std::size_t apart = 0;
    for (std::size_t i = 0; i < rays.size(); ++i) {
        const bool same = got.nearest[i].found() == want.nearest[i].found() &&
            (!want.nearest[i].found() ||
                (got.nearest[i].distance == want.nearest[i].distance && got.nearest[i].instances[0] == want.nearest[i].instances[0] &&
                    got.nearest[i].primitive == want.nearest[i].primitive));
        apart += same ? 0 : 1;
    }
    BOOST_TEST(apart == 0U);
}

// the first update builds every geometry's hierarchy, each group's and the world's
BOOST_AUTO_TEST_CASE(moya_assembler_the_first_update_builds_every_hierarchy_test) {
    const Placed placed;
    CpuDevice device;
    accel::Assembler assembler(device, {});
    assembler.update(placed.scene);
    BOOST_TEST(assembler.version() == placed.scene.version());
    BOOST_TEST((assembler.builtGeometries() == Indices { placed.left.index, placed.right.index }));
    BOOST_TEST((assembler.builtGroups() == Indices { placed.group.index, geometry::Group::kWorld }));
    agrees(device, assembler, placed.scene);

    // with nothing changed, nothing is built
    assembler.update(placed.scene);
    BOOST_TEST(assembler.builtGeometries().empty());
    BOOST_TEST(assembler.builtGroups().empty());
    agrees(device, assembler, placed.scene);
}

// moving an instance builds the hierarchy that holds it, and those that hold that, and no
// prototype's
BOOST_AUTO_TEST_CASE(moya_assembler_moving_one_instance_rebuilds_no_prototypes_hierarchy_test) {
    Placed placed;
    CpuDevice device;
    accel::Assembler assembler(device, {});
    assembler.update(placed.scene);

    placed.scene.set(placed.leftInstance, "transform", scene::Value::matrix(moved(-5.0F, 0.0F, 0.0F)));
    placed.scene.commit();
    placed.scene.apply();
    assembler.update(placed.scene);
    BOOST_TEST(assembler.builtGeometries().empty());
    BOOST_TEST((assembler.builtGroups() == Indices { geometry::Group::kWorld }));
    agrees(device, assembler, placed.scene);

    placed.scene.set(placed.rightInstance, "transform", scene::Value::matrix(moved(-1.0F, 0.0F, 0.0F)));
    placed.scene.commit();
    placed.scene.apply();
    assembler.update(placed.scene);
    BOOST_TEST(assembler.builtGeometries().empty());
    BOOST_TEST((assembler.builtGroups() == Indices { placed.group.index, geometry::Group::kWorld }));
    agrees(device, assembler, placed.scene);
}

// changing one mesh builds that mesh's hierarchy alone, with the hierarchies that place it
BOOST_AUTO_TEST_CASE(moya_assembler_changing_one_mesh_rebuilds_that_meshs_alone_test) {
    Placed placed;
    CpuDevice device;
    accel::Assembler assembler(device, {});
    assembler.update(placed.scene);

    placed.scene.set(placed.right, "P", scene::Value::points({ { 0.0F, 0.0F, 1.0F }, { 2.0F, 0.0F, 1.0F }, { 0.0F, 2.0F, 1.0F } }));
    placed.scene.commit();
    placed.scene.apply();
    assembler.update(placed.scene);
    BOOST_TEST((assembler.builtGeometries() == Indices { placed.right.index }));
    BOOST_TEST((assembler.builtGroups() == Indices { placed.group.index, geometry::Group::kWorld }));
    agrees(device, assembler, placed.scene);
}

// a removed prototype's instances place nothing, and the world is built again without them
BOOST_AUTO_TEST_CASE(moya_assembler_a_removed_prototypes_instances_are_left_out_test) {
    Placed placed;
    CpuDevice device;
    accel::Assembler assembler(device, {});
    assembler.update(placed.scene);

    placed.scene.remove(placed.left);
    placed.scene.commit();
    placed.scene.apply();
    assembler.update(placed.scene);
    BOOST_TEST(assembler.prototypes().instances.size() == 2U);
    BOOST_TEST(assembler.prototypes().geometries.size() == 1U);
    BOOST_TEST(assembler.builtGeometries().empty());
    BOOST_TEST((assembler.builtGroups() == Indices { geometry::Group::kWorld }));
    agrees(device, assembler, placed.scene);
}

// a geometry no instance places is dropped, so when an instance places it again its hierarchy
// is built again
BOOST_AUTO_TEST_CASE(moya_assembler_a_geometry_placed_by_nothing_is_dropped_test) {
    Placed placed;
    CpuDevice device;
    accel::Assembler assembler(device, {});
    assembler.update(placed.scene);

    placed.scene.remove(placed.leftInstance);
    placed.scene.commit();
    placed.scene.apply();
    assembler.update(placed.scene);
    BOOST_TEST(assembler.prototypes().geometries.size() == 1U);

    instanced(placed.scene, "left again", placed.left, {}, moved(-10.0F, 0.0F, 0.0F));
    placed.scene.commit();
    placed.scene.apply();
    assembler.update(placed.scene);
    BOOST_TEST((assembler.builtGeometries() == Indices { placed.left.index }));
    agrees(device, assembler, placed.scene);
}

// an object that is no geometry and places none, such as a light, builds nothing when it changes
BOOST_AUTO_TEST_CASE(moya_assembler_a_change_that_places_nothing_builds_nothing_test) {
    Placed placed;
    const scene::Handle light = placed.scene.create("distantLight", "sun");
    placed.scene.commit();
    placed.scene.apply();
    CpuDevice device;
    accel::Assembler assembler(device, {});
    assembler.update(placed.scene);

    placed.scene.set(light, "intensity", scene::Value::number(2.0F));
    placed.scene.commit();
    placed.scene.apply();
    assembler.update(placed.scene);
    BOOST_TEST(assembler.builtGeometries().empty());
    BOOST_TEST(assembler.builtGroups().empty());
}

// a subdivision surface is cut again, and its hierarchy built again, when the camera it is
// measured from moves. A mesh beside it is not
BOOST_AUTO_TEST_CASE(moya_assembler_a_surface_is_cut_again_when_its_dicing_camera_moves_test) {
    Placed placed;
    scene::Scene & scene = placed.scene;
    const scene::Handle options = *scene.find("options", "options");
    const scene::Handle camera = *scene.find("camera", "camera");
    scene.set(options, "resolution", scene::Value::integers({ 100, 100 }));
    scene.set(camera, "projection", scene::Value::string("perspective"));
    scene.set(camera, "transform", scene::Value::matrix(moved(0.0F, 0.0F, -5.0F)));
    const scene::Handle cage = scene.create("subdivision", "cage");
    scene.set(cage, "P", scene::Value::points({ { -1.0F, -1.0F, 0.0F }, { 1.0F, -1.0F, 0.0F }, { 1.0F, 1.0F, 0.0F }, { -1.0F, 1.0F, 0.0F } }));
    scene.set(cage, "vertices", scene::Value::integers({ 4 }));
    scene.set(cage, "indices", scene::Value::integers({ 0, 1, 2, 3 }));
    scene.set(cage, "scheme", scene::Value::string("bilinear"));
    scene.set(cage, "boundary", scene::Value::string("edges"));
    instanced(scene, "cage", cage, {}, glm::mat4(1.0F));
    scene.commit();
    scene.apply();
    CpuDevice device;
    accel::Assembler assembler(device, {});
    assembler.update(scene);
    const std::size_t near = assembler.prototypes().faces.size();

    scene.set(camera, "transform", scene::Value::matrix(moved(0.0F, 0.0F, -20.0F)));
    scene.commit();
    scene.apply();
    assembler.update(scene);
    BOOST_TEST((assembler.builtGeometries() == Indices { cage.index }));
    BOOST_TEST((assembler.builtGroups() == Indices { geometry::Group::kWorld }));
    BOOST_TEST(assembler.prototypes().faces.size() < near);
    agrees(device, assembler, scene);
}

};  // namespace
};  // namespace v3d::moya::tracing
