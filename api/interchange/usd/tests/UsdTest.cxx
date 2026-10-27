/**
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 **/

#include <api/curve/Curve.h>
#include <api/interchange/Scene.h>
#include <api/interchange/usd/Usd.h>

#include <cmath>
#include <cstddef>
#include <cstdint>
#include <optional>
#include <string>
#include <variant>
#include <vector>

#include <boost/test/unit_test.hpp>

#include <glm/geometric.hpp>
#include <glm/gtc/constants.hpp>
#include <glm/gtc/quaternion.hpp>
#include <glm/mat4x4.hpp>
#include <glm/trigonometric.hpp>
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

/**
 * A node's place in the scene: its own matrix under each of its parents' in turn.
 **/
glm::mat4 world(const Scene& scene, std::size_t node) {
    glm::mat4 matrix = scene.nodes[node].placement.matrix();
    for (std::optional<std::size_t> above = scene.nodes[node].parent; above; above = scene.nodes[*above].parent) {
        matrix = scene.nodes[*above].placement.matrix() * matrix;
    }
    return matrix;
}

Scene readScene(const std::string& path) {
    const Imported imported = v3d::interchange::readUsd(path);
    BOOST_REQUIRE_MESSAGE(imported.scene, imported.problem);
    return *imported.scene;
}

/**
 * Blender writes each object as a prim with its data as a child prim, and the stage z up. The
 * box comes out where the glTF reader puts it, one unit above the rig along y.
 **/
BOOST_AUTO_TEST_CASE(usd_reads_the_hierarchy_y_up_test) {
    const Scene scene = readScene("data/scene.usda");

    BOOST_REQUIRE_EQUAL(scene.nodes.size(), 8U);
    const std::size_t root = nodeNamed(scene, "root");
    const std::size_t rig = nodeNamed(scene, "Rig");
    const std::size_t box = nodeNamed(scene, "Box");
    const std::size_t mesh = nodeNamed(scene, "BoxMesh");
    BOOST_CHECK(!scene.nodes[root].parent);
    BOOST_CHECK_EQUAL(*scene.nodes[rig].parent, root);
    BOOST_CHECK_EQUAL(*scene.nodes[box].parent, rig);
    BOOST_CHECK_EQUAL(*scene.nodes[mesh].parent, box);
    checkVector(glm::vec3(world(scene, mesh)[3]), glm::vec3(2.0F, 1.0F, 0.0F));
}

/**
 * USD keeps a mesh's polygons, so the cube is six quads.
 **/
BOOST_AUTO_TEST_CASE(usd_reads_a_mesh_as_polygons_test) {
    const Scene scene = readScene("data/scene.usda");

    const Node& node = scene.nodes[nodeNamed(scene, "BoxMesh")];
    BOOST_REQUIRE(node.mesh);
    const v3d::interchange::Mesh& mesh = scene.meshes[*node.mesh];
    BOOST_CHECK(!mesh.triangulated);
    BOOST_CHECK_EQUAL(mesh.points.size(), 8U);
    BOOST_REQUIRE_EQUAL(mesh.counts.size(), 6U);
    for (const std::uint32_t count : mesh.counts) {
        BOOST_CHECK_EQUAL(count, 4U);
    }
}

BOOST_AUTO_TEST_CASE(usd_reads_a_camera_as_gltf_does_test) {
    const Scene scene = readScene("data/scene.usda");

    const std::size_t lens = nodeNamed(scene, "ShotLens");
    BOOST_REQUIRE(scene.nodes[lens].camera);
    const Camera& camera = scene.cameras[*scene.nodes[lens].camera];
    BOOST_CHECK(camera.projection == Camera::Projection::Perspective);
    BOOST_CHECK_CLOSE(camera.yfov, glm::radians(40.0F), 0.01F);
    BOOST_CHECK_CLOSE(camera.aspect, 16.0F / 9.0F, 0.01F);
    BOOST_CHECK_CLOSE(camera.clipping.x, 0.5F, 0.01F);
    BOOST_CHECK_CLOSE(camera.clipping.y, 200.0F, 0.01F);
    const glm::vec3 view = glm::normalize(glm::vec3(world(scene, lens) * glm::vec4(0.0F, 0.0F, -1.0F, 0.0F)));
    checkVector(view, glm::vec3(0.0F, -std::cos(glm::radians(80.0F)), -std::sin(glm::radians(80.0F))));
}

/**
 * A spot light has the power, the cone and the direction the glTF reader gives it.
 **/
BOOST_AUTO_TEST_CASE(usd_reads_a_spot_light_as_gltf_does_test) {
    const Scene scene = readScene("data/scene.usda");

    const std::size_t beam = nodeNamed(scene, "LampBeam");
    BOOST_REQUIRE(scene.nodes[beam].light);
    const Light& light = scene.lights[*scene.nodes[beam].light];
    BOOST_CHECK(light.type == Light::Type::Spot);
    checkVector(light.colour, glm::vec3(1.0F, 0.5F, 0.25F));
    BOOST_CHECK_CLOSE(light.outerCone, glm::radians(30.0F), 0.01F);
    BOOST_CHECK_CLOSE(light.innerCone, glm::radians(22.5F), 0.01F);
    BOOST_CHECK_CLOSE(light.intensity, 1000.0F / (4.0F * glm::pi<float>()), 0.1F);
    const glm::vec3 shine = glm::normalize(glm::vec3(world(scene, beam) * glm::vec4(0.0F, 0.0F, -1.0F, 0.0F)));
    checkVector(shine, glm::vec3(0.0F, -1.0F, 0.0F));
}

/**
 * The box's translation is sampled a frame a second, and its rotation and scale, which keep one
 * value, are not keyed.
 **/
BOOST_AUTO_TEST_CASE(usd_reads_keyed_translation_test) {
    const Scene scene = readScene("data/scene.usda");

    BOOST_REQUIRE_EQUAL(scene.channels.size(), 1U);
    const Channel& channel = scene.channels[0];
    BOOST_CHECK_EQUAL(channel.node, nodeNamed(scene, "Box"));
    BOOST_CHECK(channel.path == Channel::Path::Translation);
    BOOST_CHECK(channel.interpolation == Channel::Interpolation::Linear);
    BOOST_REQUIRE_EQUAL(channel.times.size(), 25U);
    BOOST_CHECK_CLOSE(channel.times.front(), 1.0F / 24.0F, 0.01F);
    BOOST_CHECK_CLOSE(channel.times.back(), 25.0F / 24.0F, 0.01F);
    BOOST_CHECK_SMALL(channel.values.front().x, TOLERANCE);
    BOOST_CHECK_CLOSE(channel.values.back().x, 4.0F, 0.01F);
}

/**
 * A keyed prim at the top of a z up stage goes beneath a group that turns and scales it, so its
 * keys keep their meaning. A still one takes the turn and the scale into its own placement.
 **/
BOOST_AUTO_TEST_CASE(usd_turns_a_z_up_stage_in_centimetres_test) {
    const Scene scene = readScene("data/keyed_root.usda");

    const std::size_t stage = nodeNamed(scene, "stage");
    const std::size_t mover = nodeNamed(scene, "Mover");
    BOOST_CHECK_EQUAL(*scene.nodes[mover].parent, stage);
    BOOST_REQUIRE_EQUAL(scene.channels.size(), 1U);
    BOOST_CHECK_CLOSE(scene.channels[0].values.back().x, 100.0F, 0.01F);
    BOOST_CHECK_CLOSE(scene.channels[0].times.back(), 1.0F, 0.01F);
    // a hundred centimetres along x is a metre along x
    checkVector(glm::vec3(scene.nodes[stage].placement.matrix() * glm::vec4(100.0F, 0.0F, 0.0F, 1.0F)), glm::vec3(1.0F, 0.0F, 0.0F));

    const std::size_t fin = nodeNamed(scene, "Fin");
    BOOST_CHECK(!scene.nodes[fin].parent);
    // a centimetre up z is a centimetre up y
    checkVector(glm::vec3(world(scene, fin) * glm::vec4(0.0F, 0.0F, 1.0F, 0.0F)), glm::vec3(0.0F, 0.01F, 0.0F));
}

/**
 * A left handed mesh is turned round so it runs counter-clockwise. A light of a kind the scene
 * cannot hold leaves a group and a note. A Scope groups, and a material beneath it is left out.
 * An orthographic camera's apertures are tenths of the stage's unit.
 **/
BOOST_AUTO_TEST_CASE(usd_reads_what_it_can_of_each_prim_test) {
    const Imported imported = v3d::interchange::readUsd("data/keyed_root.usda");
    BOOST_REQUIRE_MESSAGE(imported.scene, imported.problem);
    const Scene& scene = *imported.scene;

    const v3d::interchange::Mesh& fin = scene.meshes[*scene.nodes[nodeNamed(scene, "Fin")].mesh];
    BOOST_CHECK(fin.indices == std::vector<std::uint32_t>({ 2, 1, 0 }));

    const Node& panel = scene.nodes[nodeNamed(scene, "Panel")];
    BOOST_CHECK(!panel.light && !panel.mesh && !panel.camera);
    BOOST_REQUIRE_EQUAL(imported.notes.size(), 1U);
    BOOST_CHECK_NE(imported.notes[0].find("Panel"), std::string::npos);

    nodeNamed(scene, "Looks");
    for (const Node& node : scene.nodes) {
        BOOST_CHECK_NE(node.name, "Paint");
    }

    const Camera& flat = scene.cameras[*scene.nodes[nodeNamed(scene, "Flat")].camera];
    BOOST_CHECK(flat.projection == Camera::Projection::Orthographic);
    BOOST_CHECK_CLOSE(flat.magnification.x, 0.02F, 0.01F);
    BOOST_CHECK_CLOSE(flat.magnification.y, 0.015F, 0.01F);
    BOOST_CHECK_CLOSE(flat.clipping.x, 0.01F, 0.01F);
    BOOST_CHECK_CLOSE(flat.clipping.y, 10.0F, 0.01F);
}

BOOST_AUTO_TEST_CASE(usd_reports_a_stage_it_cannot_open_test) {
    const Imported missing = v3d::interchange::readUsd("data/missing.usda");

    BOOST_CHECK(!missing.scene);
    BOOST_CHECK_NE(missing.problem.find("missing.usda"), std::string::npos);
}

/**
 * Each kind of BasisCurves prim, and a NurbsCurves prim, read as curves of the type that is the
 * same curve, at the mean of their widths. A prim of a basis the scene cannot hold is a group,
 * and a curve whose points make no curve is left out. Each of those is noted.
 **/
BOOST_AUTO_TEST_CASE(usd_reads_curves_test) {
    const Imported imported = v3d::interchange::readUsd("data/curves.usda");
    BOOST_REQUIRE_MESSAGE(imported.scene, imported.problem);
    const Scene& scene = *imported.scene;
    const auto curvesOf = [&scene](const std::string& name) -> const v3d::interchange::Curves& {
        const Node& node = scene.nodes[nodeNamed(scene, name)];
        BOOST_REQUIRE_MESSAGE(node.curves, name << " has no curves");
        return scene.curves[*node.curves];
    };

    // two closed poly curves, at the constant width
    const v3d::interchange::Curves& lines = curvesOf("Lines");
    BOOST_REQUIRE_EQUAL(lines.curves.size(), 2U);
    BOOST_CHECK(std::holds_alternative<v3d::curve::Poly>(lines.curves[1].shape));
    BOOST_CHECK(lines.curves[1].closed);
    BOOST_CHECK_CLOSE(lines.width, 0.25F, 0.001F);

    // a Bezier curve of three points, whose handles are the points between
    const v3d::interchange::Curves& bezier = curvesOf("Bezier");
    BOOST_REQUIRE_EQUAL(bezier.curves.size(), 1U);
    const v3d::curve::Bezier& handles = std::get<v3d::curve::Bezier>(bezier.curves[0].shape);
    BOOST_REQUIRE_EQUAL(handles.points.size(), 3U);
    checkVector(handles.points[1].position, glm::vec3(3, 0, 0));
    checkVector(handles.points[1].left, glm::vec3(2, 1, 0));
    checkVector(handles.points[1].right, glm::vec3(4, -1, 0));

    // a Catmull-Rom curve runs from its second point to its second to last, through each of them
    const std::vector<glm::vec3> catmullRom = v3d::curve::evaluate(curvesOf("CatmullRom").curves[0], 4);
    BOOST_REQUIRE_EQUAL(catmullRom.size(), 9U);
    checkVector(catmullRom.front(), glm::vec3(1, 1, 0));
    checkVector(catmullRom[4], glm::vec3(2, 0, 0));
    checkVector(catmullRom.back(), glm::vec3(3, 1, 0));
    // and between them it is the uniform Catmull-Rom segment: halfway from (1, 1) to (2, 0)
    checkVector(catmullRom[2], glm::vec3(1.5F, 0.5F, 0.0F));

    // a pinned B-spline starts and ends on its end points
    const std::vector<glm::vec3> pinned = v3d::curve::evaluate(curvesOf("Pinned").curves[0], 4);
    checkVector(pinned.front(), glm::vec3(0, 0, 0));
    checkVector(pinned.back(), glm::vec3(3, 0, 0));

    // a periodic B-spline is closed, and returns to where it starts
    const v3d::curve::Curve& loop = curvesOf("Loop").curves[0];
    BOOST_CHECK(loop.closed);
    const std::vector<glm::vec3> around = v3d::curve::evaluate(loop, 4);
    checkVector(around.back(), around.front());

    // a weighted NURBS circle keeps its weights, and is round
    for (const glm::vec3& point : v3d::curve::evaluate(curvesOf("Circle").curves[0], 8)) {
        BOOST_CHECK_SMALL(glm::length(point) - 1.0F, TOLERANCE);
    }

    // one of the broken prim's curves has a count no Bezier curve has, and the hermite prim's
    // basis is not one the scene holds
    BOOST_CHECK_EQUAL(curvesOf("Broken").curves.size(), 1U);
    BOOST_CHECK(!scene.nodes[nodeNamed(scene, "Hermite")].curves);
    BOOST_CHECK_EQUAL(imported.notes.size(), 2U);
}

};  // namespace
