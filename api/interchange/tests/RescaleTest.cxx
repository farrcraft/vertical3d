/**
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 **/

#include <api/curve/Curve.h>
#include <api/interchange/Rescale.h>
#include <api/interchange/Scene.h>

#include <variant>

#include <boost/test/unit_test.hpp>
#include <glm/gtc/quaternion.hpp>
#include <glm/vec2.hpp>
#include <glm/vec3.hpp>
#include <glm/vec4.hpp>

namespace {

BOOST_AUTO_TEST_SUITE(rescale_test)

/**
 * Every length is multiplied by the factor, and what is not a length is kept: a rotation, a
 * scale, a rotation channel, a NURBS curve's weights and a light's power.
 **/
BOOST_AUTO_TEST_CASE(every_length_is_scaled_and_nothing_else) {
    v3d::interchange::Scene scene;
    scene.meshes.push_back({ .name = "box", .points = { glm::vec3(1.0F, 2.0F, 3.0F) }, .counts = {}, .indices = {}, .triangulated = false });
    v3d::interchange::Node node;
    node.placement.position = glm::vec3(4.0F, 0.0F, 0.0F);
    node.placement.scale = glm::vec3(2.0F);
    node.placement.rotation = glm::angleAxis(0.5F, glm::vec3(0.0F, 1.0F, 0.0F));
    scene.nodes.push_back(node);
    v3d::interchange::Channel moved;
    moved.path = v3d::interchange::Channel::Path::Translation;
    moved.values = { glm::vec4(1.0F, 1.0F, 1.0F, 7.0F) };
    moved.inTangents = { glm::vec4(2.0F) };
    moved.outTangents = { glm::vec4(3.0F) };
    v3d::interchange::Channel turned;
    turned.path = v3d::interchange::Channel::Path::Rotation;
    turned.values = { glm::vec4(0.0F, 0.0F, 0.0F, 1.0F) };
    scene.channels = { moved, turned };
    v3d::curve::Nurbs nurbs;
    nurbs.points = { glm::vec3(1.0F) };
    nurbs.weights = { 0.5F };
    v3d::curve::Bezier bezier;
    bezier.points.push_back({ .position = glm::vec3(1.0F), .left = glm::vec3(2.0F), .right = glm::vec3(3.0F) });
    scene.curves.push_back({ .name = "path", .curves = { { .shape = nurbs, .closed = false }, { .shape = bezier, .closed = false } }, .width = 0.1F });
    v3d::interchange::Camera camera;
    camera.clipping = glm::vec2(0.1F, 100.0F);
    camera.magnification = glm::vec2(5.0F, 4.0F);
    scene.cameras.push_back(camera);
    v3d::interchange::Light light;
    light.intensity = 30.0F;
    scene.lights.push_back(light);

    v3d::interchange::rescale(&scene, 100.0F);

    BOOST_CHECK(scene.meshes[0].points[0] == glm::vec3(100.0F, 200.0F, 300.0F));
    BOOST_CHECK(scene.nodes[0].placement.position == glm::vec3(400.0F, 0.0F, 0.0F));
    BOOST_CHECK(scene.nodes[0].placement.scale == glm::vec3(2.0F));
    BOOST_CHECK(scene.nodes[0].placement.rotation == node.placement.rotation);
    BOOST_CHECK(scene.channels[0].values[0] == glm::vec4(100.0F, 100.0F, 100.0F, 7.0F));
    BOOST_CHECK(scene.channels[0].inTangents[0] == glm::vec4(200.0F, 200.0F, 200.0F, 2.0F));
    BOOST_CHECK(scene.channels[0].outTangents[0] == glm::vec4(300.0F, 300.0F, 300.0F, 3.0F));
    BOOST_CHECK(scene.channels[1].values[0] == glm::vec4(0.0F, 0.0F, 0.0F, 1.0F));
    const auto& scaledNurbs = std::get<v3d::curve::Nurbs>(scene.curves[0].curves[0].shape);
    BOOST_CHECK(scaledNurbs.points[0] == glm::vec3(100.0F));
    BOOST_CHECK_EQUAL(scaledNurbs.weights[0], 0.5F);
    const auto& scaledBezier = std::get<v3d::curve::Bezier>(scene.curves[0].curves[1].shape);
    BOOST_CHECK(scaledBezier.points[0].left == glm::vec3(200.0F));
    BOOST_CHECK(scaledBezier.points[0].right == glm::vec3(300.0F));
    BOOST_CHECK_CLOSE(scene.curves[0].width, 10.0F, 0.001F);
    BOOST_CHECK_CLOSE(scene.cameras[0].clipping.x, 10.0F, 0.001F);
    BOOST_CHECK_CLOSE(scene.cameras[0].clipping.y, 10000.0F, 0.001F);
    BOOST_CHECK(scene.cameras[0].magnification == glm::vec2(500.0F, 400.0F));
    BOOST_CHECK_EQUAL(scene.lights[0].intensity, 30.0F);
}

BOOST_AUTO_TEST_SUITE_END()

};  // namespace
