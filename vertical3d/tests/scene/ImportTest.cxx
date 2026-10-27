/**
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 **/

#include <api/animation/Curve.h>
#include <api/interchange/Scene.h>
#include <vertical3d/src/scene/Camera.h>
#include <vertical3d/src/scene/Import.h>
#include <vertical3d/src/scene/Item.h>
#include <vertical3d/src/scene/Light.h>
#include <vertical3d/src/scene/Mesh.h>
#include <vertical3d/src/scene/Properties.h>
#include <vertical3d/src/scene/Scene.h>

#include <cmath>
#include <cstddef>
#include <optional>
#include <string>

#include <boost/test/unit_test.hpp>

#include <glm/ext/quaternion_float.hpp>
#include <glm/ext/quaternion_trigonometric.hpp>
#include <glm/gtc/constants.hpp>
#include <glm/trigonometric.hpp>
#include <glm/vec2.hpp>
#include <glm/vec3.hpp>
#include <glm/vec4.hpp>

namespace {

using v3d::interchange::Channel;
using v3d::interchange::Node;
using v3d::interchange::Scene;

const float TOLERANCE = 0.0001F;

void checkVector(const glm::vec3& actual, const glm::vec3& expected) {
    BOOST_CHECK_SMALL(actual.x - expected.x, TOLERANCE);
    BOOST_CHECK_SMALL(actual.y - expected.y, TOLERANCE);
    BOOST_CHECK_SMALL(actual.z - expected.z, TOLERANCE);
}

/**
 * A unit square in the xy plane facing +z, cut into two triangles as glTF stores it.
 **/
v3d::interchange::Mesh square() {
    v3d::interchange::Mesh mesh;
    mesh.name = "square";
    mesh.points = { glm::vec3(0, 0, 0), glm::vec3(1, 0, 0), glm::vec3(1, 1, 0), glm::vec3(0, 1, 0) };
    mesh.counts = { 3, 3 };
    mesh.indices = { 0, 1, 2, 0, 2, 3 };
    mesh.triangulated = true;
    return mesh;
}

Node node(const std::string& name, std::optional<std::size_t> parent) {
    Node made;
    made.name = name;
    made.parent = parent;
    return made;
}

BOOST_AUTO_TEST_SUITE(import_test)

/**
 * Each node becomes one item of its kind, a node with nothing becomes a group, and the items
 * keep the nodes' hierarchy. A triangulated mesh is joined back into its quads.
 **/
BOOST_AUTO_TEST_CASE(nodes_become_items_of_their_kind) {
    Scene scene;
    scene.meshes.push_back(square());
    scene.cameras.emplace_back();
    scene.cameras.back().yfov = glm::radians(40.0F);
    scene.cameras.back().clipping.x = 0.1F;
    scene.lights.emplace_back();
    scene.nodes.push_back(node("rig", std::nullopt));
    scene.nodes.push_back(node("box", 0));
    scene.nodes.back().mesh = 0;
    scene.nodes.push_back(node("shot", 0));
    scene.nodes.back().camera = 0;
    scene.nodes.push_back(node("lamp", std::nullopt));
    scene.nodes.back().light = 0;

    const v3d::editor::Import imported = v3d::editor::importScene(scene, 24.0F);

    BOOST_REQUIRE_EQUAL(imported.items.size(), 4U);
    BOOST_CHECK(imported.items[0]->kind() == v3d::editor::Item::Kind::Group);
    BOOST_CHECK(imported.items[1]->kind() == v3d::editor::Item::Kind::Mesh);
    BOOST_CHECK(imported.items[2]->kind() == v3d::editor::Item::Kind::Camera);
    BOOST_CHECK(imported.items[3]->kind() == v3d::editor::Item::Kind::Light);
    BOOST_CHECK(!imported.parents[0]);
    BOOST_CHECK_EQUAL(*imported.parents[1], 0U);
    BOOST_CHECK_EQUAL(*imported.parents[2], 0U);
    BOOST_CHECK(!imported.parents[3]);
    const boost::shared_ptr<v3d::editor::Mesh> mesh = v3d::editor::as<v3d::editor::Mesh>(imported.items[1]);
    BOOST_CHECK_EQUAL(mesh->geometry()->faceCount(), 1U);
    BOOST_CHECK(imported.notes.empty());
}

/**
 * A file's camera looks along -z and the editor's along +z, so an imported camera is turned
 * half a turn. A child of the camera's node stays where the file put it, and so does the
 * camera's view under a key.
 **/
BOOST_AUTO_TEST_CASE(a_camera_is_turned_to_look_where_the_file_says) {
    Scene scene;
    scene.cameras.emplace_back();
    scene.cameras.back().yfov = glm::radians(40.0F);
    scene.cameras.back().clipping.x = 0.1F;
    scene.nodes.push_back(node("shot", std::nullopt));
    scene.nodes.back().camera = 0;
    scene.nodes.push_back(node("marker", 0));
    scene.nodes.back().placement.position = glm::vec3(1.0F, 2.0F, 3.0F);
    Channel turn;
    turn.node = 0;
    turn.path = Channel::Path::Rotation;
    turn.times = { 0.0F, 1.0F };
    turn.values = { glm::vec4(0, 0, 0, 1), glm::vec4(0, 0, 0, 1) };
    scene.channels.push_back(turn);

    const v3d::editor::Import imported = v3d::editor::importScene(scene, 24.0F);

    BOOST_REQUIRE_EQUAL(imported.items.size(), 2U);
    const boost::shared_ptr<v3d::editor::Item>& camera = imported.items[0];
    checkVector(camera->rotation() * glm::vec3(0.0F, 0.0F, 1.0F), glm::vec3(0.0F, 0.0F, -1.0F));
    checkVector(camera->rotation() * glm::vec3(0.0F, 1.0F, 0.0F), glm::vec3(0.0F, 1.0F, 0.0F));
    // the marker's place in the world is the camera's matrix applied to its own position
    const glm::vec3 marker = glm::vec3(camera->matrix() * glm::vec4(imported.items[1]->translation(), 1.0F));
    checkVector(marker, glm::vec3(1.0F, 2.0F, 3.0F));

    const v3d::editor::Curves& curves = imported.curves[0];
    BOOST_REQUIRE_EQUAL(curves.count("rotation.x") + curves.count("rotation.y") + curves.count("rotation.z"), 3U);
    const glm::vec3 angles(curves.at("rotation.x").sample(0.0F), curves.at("rotation.y").sample(0.0F),
        curves.at("rotation.z").sample(0.0F));
    checkVector(v3d::editor::fromEuler(angles) * glm::vec3(0.0F, 0.0F, 1.0F), glm::vec3(0.0F, 0.0F, -1.0F));
}

/**
 * Two nodes that use one mesh give two items that share one geometry.
 **/
BOOST_AUTO_TEST_CASE(nodes_that_share_a_mesh_share_its_geometry) {
    Scene scene;
    scene.meshes.push_back(square());
    scene.nodes.push_back(node("one", std::nullopt));
    scene.nodes.back().mesh = 0;
    scene.nodes.push_back(node("two", std::nullopt));
    scene.nodes.back().mesh = 0;

    const v3d::editor::Import imported = v3d::editor::importScene(scene, 24.0F);

    BOOST_REQUIRE_EQUAL(imported.items.size(), 2U);
    BOOST_CHECK(v3d::editor::as<v3d::editor::Mesh>(imported.items[0])->geometry() ==
        v3d::editor::as<v3d::editor::Mesh>(imported.items[1])->geometry());
}

/**
 * A mesh that cannot be built is noted, and its node is a group. A node that carries a mesh and
 * a light is the mesh, with the light as its child.
 **/
BOOST_AUTO_TEST_CASE(a_node_keeps_what_it_can) {
    Scene scene;
    v3d::interchange::Mesh fin = square();
    fin.name = "fin";
    // two triangles running the same way along one edge
    fin.indices = { 0, 1, 2, 0, 1, 3 };
    fin.triangulated = false;
    scene.meshes.push_back(fin);
    scene.meshes.push_back(square());
    scene.lights.emplace_back();
    scene.nodes.push_back(node("broken", std::nullopt));
    scene.nodes.back().mesh = 0;
    scene.nodes.push_back(node("lit", std::nullopt));
    scene.nodes.back().mesh = 1;
    scene.nodes.back().light = 0;

    const v3d::editor::Import imported = v3d::editor::importScene(scene, 24.0F);

    BOOST_REQUIRE_EQUAL(imported.items.size(), 3U);
    BOOST_CHECK(imported.items[0]->kind() == v3d::editor::Item::Kind::Group);
    BOOST_REQUIRE_EQUAL(imported.notes.size(), 1U);
    BOOST_CHECK_NE(imported.notes[0].find("fin"), std::string::npos);
    BOOST_CHECK(imported.items[1]->kind() == v3d::editor::Item::Kind::Mesh);
    BOOST_CHECK(imported.items[2]->kind() == v3d::editor::Item::Kind::Light);
    BOOST_CHECK_EQUAL(*imported.parents[2], 1U);
    // the light shines along its mesh's -z, as the file's light did along the node's
    checkVector(imported.items[2]->rotation() * glm::vec3(0.0F, 0.0F, 1.0F), glm::vec3(0.0F, 0.0F, -1.0F));
}

/**
 * A key in seconds goes on the frame at that time. Linear keys stay linear, and a cubic
 * spline's tangents, a change a second, become slopes, a change a frame.
 **/
BOOST_AUTO_TEST_CASE(keys_go_on_frames_at_the_timeline_rate) {
    Scene scene;
    scene.nodes.push_back(node("mover", std::nullopt));
    Channel move;
    move.node = 0;
    move.path = Channel::Path::Translation;
    move.times = { 0.5F, 1.5F };
    move.values = { glm::vec4(0, 0, 0, 0), glm::vec4(4, 0, 0, 0) };
    scene.channels.push_back(move);
    Channel grow;
    grow.node = 0;
    grow.path = Channel::Path::Scale;
    grow.interpolation = Channel::Interpolation::CubicSpline;
    grow.times = { 0.0F, 1.0F };
    grow.values = { glm::vec4(1, 1, 1, 0), glm::vec4(2, 2, 2, 0) };
    grow.inTangents = { glm::vec4(0), glm::vec4(48, 0, 0, 0) };
    grow.outTangents = { glm::vec4(24, 0, 0, 0), glm::vec4(0) };
    scene.channels.push_back(grow);

    const v3d::editor::Import imported = v3d::editor::importScene(scene, 24.0F);

    const v3d::editor::Curves& curves = imported.curves[0];
    const v3d::animation::Curve& x = curves.at("translation.x");
    BOOST_REQUIRE_EQUAL(x.keys().size(), 2U);
    BOOST_CHECK_CLOSE(x.keys()[0].frame, 12.0F, 0.01F);
    BOOST_CHECK_CLOSE(x.keys()[1].frame, 36.0F, 0.01F);
    BOOST_CHECK(x.keys()[0].interpolation == v3d::animation::Curve::Interpolation::Linear);
    BOOST_CHECK_CLOSE(x.sample(24.0F), 2.0F, 0.01F);

    const v3d::animation::Curve& scaleX = curves.at("scale.x");
    BOOST_REQUIRE_EQUAL(scaleX.keys().size(), 2U);
    BOOST_CHECK(scaleX.keys()[0].interpolation == v3d::animation::Curve::Interpolation::Bezier);
    BOOST_CHECK(!scaleX.keys()[0].automatic);
    BOOST_CHECK_CLOSE(scaleX.keys()[0].out, 1.0F, 0.01F);
    BOOST_CHECK_CLOSE(scaleX.keys()[1].in, 2.0F, 0.01F);
}

/**
 * A perspective angle runs across the shorter side in the editor, so a tall picture's angle is
 * its width's. An orthographic camera keeps its half height, and a camera with no far plane
 * gets a new camera's. A spot light's soft edge is the gap between its two angles.
 **/
BOOST_AUTO_TEST_CASE(a_lens_and_an_emission_are_the_editors) {
    Scene scene;
    scene.cameras.emplace_back();
    scene.cameras.back().yfov = glm::radians(90.0F);
    scene.cameras.back().aspect = 0.5F;
    scene.cameras.back().clipping.x = 0.5F;
    scene.cameras.emplace_back();
    scene.cameras.back().projection = v3d::interchange::Camera::Projection::Orthographic;
    scene.cameras.back().magnification = glm::vec2(4.0F, 3.0F);
    scene.cameras.back().clipping.x = 0.5F;
    scene.cameras.back().clipping.y = 20.0F;
    scene.lights.emplace_back();
    scene.lights.back().type = v3d::interchange::Light::Type::Spot;
    scene.lights.back().outerCone = 0.5F;
    scene.lights.back().innerCone = 0.375F;
    for (std::size_t index = 0; index < 2; index++) {
        scene.nodes.push_back(node("camera", std::nullopt));
        scene.nodes.back().camera = index;
    }
    scene.nodes.push_back(node("spot", std::nullopt));
    scene.nodes.back().light = 0;

    const v3d::editor::Import imported = v3d::editor::importScene(scene, 24.0F);

    BOOST_REQUIRE_EQUAL(imported.items.size(), 3U);
    const v3d::editor::Camera::Lens& tall = v3d::editor::as<v3d::editor::Camera>(imported.items[0])->lens();
    BOOST_CHECK_CLOSE(tall.fov, glm::degrees(2.0F * std::atan(0.5F)), 0.01F);
    BOOST_CHECK_CLOSE(tall.clipping.y, v3d::editor::Camera::defaultFar, 0.01F);
    const v3d::editor::Camera::Lens& flat = v3d::editor::as<v3d::editor::Camera>(imported.items[1])->lens();
    BOOST_CHECK(flat.projection == v3d::editor::Camera::Projection::Orthographic);
    BOOST_CHECK_CLOSE(flat.extent, 3.0F, 0.01F);
    BOOST_CHECK_CLOSE(flat.clipping.y, 20.0F, 0.01F);
    const v3d::editor::Light::Emission& spot = v3d::editor::as<v3d::editor::Light>(imported.items[2])->emission();
    BOOST_CHECK(spot.type == v3d::editor::Light::Type::Spot);
    BOOST_CHECK_CLOSE(spot.cone, 0.5F, 0.01F);
    BOOST_CHECK_CLOSE(spot.delta, 0.125F, 0.01F);
}

BOOST_AUTO_TEST_SUITE_END()

};  // namespace
