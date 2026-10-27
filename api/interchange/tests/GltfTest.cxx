/**
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 **/

#include <api/interchange/Gltf.h>
#include <api/interchange/Scene.h>

#include <cmath>
#include <cstddef>
#include <cstdint>
#include <string>

#include <boost/test/unit_test.hpp>

#include <glm/gtc/constants.hpp>
#include <glm/gtc/quaternion.hpp>
#include <glm/vec3.hpp>
#include <glm/vec4.hpp>

namespace {

using v3d::interchange::Camera;
using v3d::interchange::Channel;
using v3d::interchange::Imported;
using v3d::interchange::Light;
using v3d::interchange::Node;
using v3d::interchange::Scene;

const float TOLERANCE = 0.0001F;

/**
 * The node of a scene with a name, which the test requires to be there.
 **/
std::size_t nodeNamed(const Scene& scene, const std::string& name) {
    for (std::size_t index = 0; index < scene.nodes.size(); index++) {
        if (scene.nodes[index].name == name) {
            return index;
        }
    }
    BOOST_FAIL("no node is named " + name);
    return 0;
}

void checkVector(const glm::vec3& actual, const glm::vec3& expected) {
    BOOST_CHECK_SMALL(actual.x - expected.x, TOLERANCE);
    BOOST_CHECK_SMALL(actual.y - expected.y, TOLERANCE);
    BOOST_CHECK_SMALL(actual.z - expected.z, TOLERANCE);
}

Scene readScene(const std::string& path) {
    const Imported imported = v3d::interchange::readGltf(path);
    BOOST_REQUIRE_MESSAGE(imported.scene, imported.problem);
    return *imported.scene;
}

BOOST_AUTO_TEST_CASE(gltf_reads_the_hierarchy_parents_first_test) {
    const Scene scene = readScene("data/scene.glb");

    BOOST_REQUIRE_EQUAL(scene.nodes.size(), 4U);
    const std::size_t rig = nodeNamed(scene, "Rig");
    const std::size_t box = nodeNamed(scene, "Box");
    BOOST_CHECK(!scene.nodes[rig].parent);
    BOOST_REQUIRE(scene.nodes[box].parent);
    BOOST_CHECK_EQUAL(*scene.nodes[box].parent, rig);
    BOOST_CHECK_LT(rig, box);
    // Blender's z up becomes glTF's y up, so one unit above the rig is +y
    checkVector(scene.nodes[rig].placement.position, glm::vec3(2.0F, 0.0F, 0.0F));
    checkVector(scene.nodes[box].placement.position, glm::vec3(0.0F, 1.0F, 0.0F));
    BOOST_CHECK(!scene.nodes[rig].mesh && !scene.nodes[rig].camera && !scene.nodes[rig].light);
}

BOOST_AUTO_TEST_CASE(gltf_reads_a_mesh_as_triangles_test) {
    const Scene scene = readScene("data/scene.glb");

    const Node& box = scene.nodes[nodeNamed(scene, "Box")];
    BOOST_REQUIRE(box.mesh);
    const v3d::interchange::Mesh& mesh = scene.meshes[*box.mesh];
    BOOST_CHECK_EQUAL(mesh.name, "BoxMesh");
    BOOST_CHECK(mesh.triangulated);
    BOOST_CHECK_EQUAL(mesh.counts.size(), 12U);
    BOOST_CHECK_EQUAL(mesh.indices.size(), 36U);
    for (const std::uint32_t count : mesh.counts) {
        BOOST_CHECK_EQUAL(count, 3U);
    }
    for (const std::uint32_t index : mesh.indices) {
        BOOST_CHECK_LT(index, mesh.points.size());
    }
    for (const glm::vec3& point : mesh.points) {
        BOOST_CHECK_SMALL(std::abs(point.x) - 0.5F, TOLERANCE);
    }
}

BOOST_AUTO_TEST_CASE(gltf_reads_a_perspective_camera_test) {
    const Scene scene = readScene("data/scene.glb");

    const Node& shot = scene.nodes[nodeNamed(scene, "Shot")];
    BOOST_REQUIRE(shot.camera);
    const Camera& camera = scene.cameras[*shot.camera];
    BOOST_CHECK(camera.projection == Camera::Projection::Perspective);
    BOOST_CHECK_CLOSE(camera.yfov, glm::radians(40.0F), 0.01F);
    BOOST_CHECK_CLOSE(camera.aspect, 16.0F / 9.0F, 0.01F);
    BOOST_CHECK_CLOSE(camera.near, 0.5F, 0.01F);
    BOOST_CHECK_CLOSE(camera.far, 200.0F, 0.01F);
    // the camera looks along its node's -z, which Blender turned 80 degrees up from straight down
    const glm::vec3 view = shot.placement.rotation * glm::vec3(0.0F, 0.0F, -1.0F);
    checkVector(view, glm::vec3(0.0F, -std::cos(glm::radians(80.0F)), -std::sin(glm::radians(80.0F))));
}

BOOST_AUTO_TEST_CASE(gltf_reads_a_spot_light_test) {
    const Scene scene = readScene("data/scene.glb");

    const Node& lamp = scene.nodes[nodeNamed(scene, "Lamp")];
    BOOST_REQUIRE(lamp.light);
    const Light& light = scene.lights[*lamp.light];
    BOOST_CHECK(light.type == Light::Type::Spot);
    checkVector(light.colour, glm::vec3(1.0F, 0.5F, 0.25F));
    BOOST_CHECK_CLOSE(light.outerCone, glm::radians(30.0F), 0.01F);
    BOOST_CHECK_CLOSE(light.innerCone, glm::radians(22.5F), 0.01F);
    BOOST_CHECK_GT(light.intensity, 0.0F);
    // the light shines along its node's -z, which points down
    checkVector(lamp.placement.rotation * glm::vec3(0.0F, 0.0F, -1.0F), glm::vec3(0.0F, -1.0F, 0.0F));
}

BOOST_AUTO_TEST_CASE(gltf_reads_keyed_translation_test) {
    const Scene scene = readScene("data/scene.glb");

    BOOST_REQUIRE_EQUAL(scene.channels.size(), 1U);
    const Channel& channel = scene.channels[0];
    BOOST_CHECK_EQUAL(channel.node, nodeNamed(scene, "Box"));
    BOOST_CHECK(channel.path == Channel::Path::Translation);
    BOOST_CHECK(channel.interpolation == Channel::Interpolation::Linear);
    BOOST_REQUIRE_GE(channel.times.size(), 2U);
    BOOST_REQUIRE_EQUAL(channel.values.size(), channel.times.size());
    BOOST_CHECK_CLOSE(channel.times.back() - channel.times.front(), 1.0F, 0.01F);
    BOOST_CHECK_SMALL(channel.values.front().x, TOLERANCE);
    BOOST_CHECK_CLOSE(channel.values.back().x, 4.0F, 0.01F);
    BOOST_CHECK_CLOSE(channel.values.back().y, 1.0F, 0.01F);
}

BOOST_AUTO_TEST_CASE(gltf_takes_a_matrix_apart_test) {
    const Scene scene = readScene("data/nodes.gltf");

    BOOST_REQUIRE_EQUAL(scene.nodes.size(), 4U);
    const Node& matrix = scene.nodes[0];
    BOOST_CHECK_EQUAL(matrix.name, "Matrix");
    checkVector(matrix.placement.position, glm::vec3(1.0F, 2.0F, 3.0F));
    checkVector(matrix.placement.scale, glm::vec3(2.0F, 3.0F, 3.0F));
    // a quarter turn about x takes +y to +z
    checkVector(matrix.placement.rotation * glm::vec3(0.0F, 1.0F, 0.0F), glm::vec3(0.0F, 0.0F, 1.0F));

    // a mirror keeps its handedness in the x scale, and has no turn
    const Node& mirror = scene.nodes[nodeNamed(scene, "Mirror")];
    checkVector(mirror.placement.scale, glm::vec3(-1.0F, 1.0F, 1.0F));
    BOOST_CHECK_CLOSE(std::abs(mirror.placement.rotation.w), 1.0F, 0.01F);
}

BOOST_AUTO_TEST_CASE(gltf_reads_an_orthographic_camera_and_a_distant_light_test) {
    const Scene scene = readScene("data/nodes.gltf");

    const Node& ortho = scene.nodes[nodeNamed(scene, "Ortho")];
    const Node& sun = scene.nodes[nodeNamed(scene, "Sun")];
    BOOST_CHECK_EQUAL(*ortho.parent, 0U);
    BOOST_CHECK_EQUAL(*sun.parent, 0U);
    BOOST_REQUIRE(ortho.camera);
    const Camera& camera = scene.cameras[*ortho.camera];
    BOOST_CHECK(camera.projection == Camera::Projection::Orthographic);
    BOOST_CHECK_CLOSE(camera.magnification.x, 4.0F, 0.01F);
    BOOST_CHECK_CLOSE(camera.magnification.y, 3.0F, 0.01F);
    BOOST_CHECK_CLOSE(camera.far, 50.0F, 0.01F);
    BOOST_REQUIRE(sun.light);
    const Light& light = scene.lights[*sun.light];
    BOOST_CHECK(light.type == Light::Type::Distant);
    BOOST_CHECK_CLOSE(light.intensity, 3.0F, 0.01F);
    BOOST_CHECK_EQUAL(light.name, "SunBeam");
}

BOOST_AUTO_TEST_CASE(gltf_reports_a_file_it_cannot_read_test) {
    const Imported missing = v3d::interchange::readGltf("data/missing.glb");
    const Imported notGltf = v3d::interchange::readGltf("data/make_scene_fixture.py");

    BOOST_CHECK(!missing.scene);
    BOOST_CHECK_NE(missing.problem.find("missing.glb"), std::string::npos);
    BOOST_CHECK(!notGltf.scene);
    BOOST_CHECK(!notGltf.problem.empty());
}

};  // namespace
