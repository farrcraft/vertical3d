/**
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 **/

#include <api/render/realtime/canvas/Line.h>
#include <vertical3d/src/scene/Camera.h>
#include <vertical3d/src/scene/Glyphs.h>
#include <vertical3d/src/scene/Light.h>

#include <algorithm>
#include <cmath>
#include <cstddef>

#include <boost/test/unit_test.hpp>
#include <glm/geometric.hpp>
#include <glm/gtc/quaternion.hpp>
#include <glm/trigonometric.hpp>
#include <glm/vec3.hpp>
#include <glm/vec4.hpp>

namespace {

constexpr glm::vec4 colour(1.0F, 1.0F, 1.0F, 1.0F);

/**
 * How many segments a canvas holds, two vertices to each.
 **/
std::size_t segments(const v3d::render::realtime::canvas::Line& canvas) {
    return canvas.vertices().size() / 2;
}

/**
 * The largest of one coordinate over every vertex.
 **/
float highest(const v3d::render::realtime::canvas::Line& canvas, int axis) {
    float most = -1.0e9F;
    for (const v3d::render::realtime::canvas::Line::Vertex& vertex : canvas.vertices()) {
        most = std::max(most, vertex.position[axis]);
    }
    return most;
}

BOOST_AUTO_TEST_CASE(glyph_perspective_camera_test) {
    v3d::render::realtime::canvas::Line canvas;
    v3d::editor::Camera::Lens lens;
    lens.fov = 60.0F;
    v3d::editor::drawCamera(lens, colour, &canvas);

    // the picture, the four sides from the eye, and the triangle above the picture
    BOOST_CHECK_EQUAL(segments(canvas), 11U);

    // the picture is one unit along +z, as tall as the angle of view and 4:3 wide, and every
    // side starts at the eye
    const float halfHeight = std::tan(glm::radians(30.0F));
    BOOST_CHECK_CLOSE(highest(canvas, 2), 1.0F, 0.001F);
    BOOST_CHECK_CLOSE(highest(canvas, 0), halfHeight * 4.0F / 3.0F, 0.001F);
    std::size_t fromEye = 0;
    for (std::size_t index = 0; index < canvas.vertices().size(); index += 2) {
        const glm::vec3& from = canvas.vertices()[index].position;
        const glm::vec3& to = canvas.vertices()[index + 1].position;
        if (glm::length(from) < 0.0001F) {
            fromEye++;
            BOOST_CHECK_CLOSE(std::abs(to.y), halfHeight, 0.001F);
            BOOST_CHECK_CLOSE(to.z, 1.0F, 0.001F);
        }
    }
    BOOST_CHECK_EQUAL(fromEye, 4U);

    // the triangle above the picture points up, along +y
    BOOST_CHECK_GT(highest(canvas, 1), halfHeight);
}

BOOST_AUTO_TEST_CASE(glyph_orthographic_camera_test) {
    v3d::render::realtime::canvas::Line canvas;
    v3d::editor::Camera::Lens lens;
    lens.projection = v3d::editor::Camera::Projection::Orthographic;
    lens.extent = 3.0F;
    v3d::editor::drawCamera(lens, colour, &canvas);

    // a picture at each end, four sides joining them, and the triangle
    BOOST_CHECK_EQUAL(segments(canvas), 15U);
    BOOST_CHECK_CLOSE(highest(canvas, 0), 4.0F, 0.001F);
    BOOST_CHECK_CLOSE(highest(canvas, 2), 1.0F, 0.001F);
    for (const v3d::render::realtime::canvas::Line::Vertex& vertex : canvas.vertices()) {
        // nothing comes nearer than the picture at the camera itself
        BOOST_CHECK_GE(vertex.position.z, -0.0001F);
    }
}

BOOST_AUTO_TEST_CASE(glyph_point_and_distant_light_test) {
    v3d::render::realtime::canvas::Line point;
    v3d::editor::drawLight({ .type = v3d::editor::Light::Type::Point }, colour, &point);

    // three lines through the light, a quarter unit each way
    BOOST_CHECK_EQUAL(segments(point), 3U);
    for (const v3d::render::realtime::canvas::Line::Vertex& vertex : point.vertices()) {
        BOOST_CHECK_CLOSE(glm::length(vertex.position), 0.25F, 0.001F);
    }

    v3d::render::realtime::canvas::Line distant;
    v3d::editor::drawLight({ .type = v3d::editor::Light::Type::Distant }, colour, &distant);

    // three rays and the arrowhead's two strokes, all between the light and one unit along +z
    BOOST_CHECK_EQUAL(segments(distant), 5U);
    for (const v3d::render::realtime::canvas::Line::Vertex& vertex : distant.vertices()) {
        BOOST_CHECK_GE(vertex.position.z, -0.0001F);
        BOOST_CHECK_LE(vertex.position.z, 1.0001F);
    }
    BOOST_CHECK_CLOSE(highest(distant, 2), 1.0F, 0.001F);
}

BOOST_AUTO_TEST_CASE(glyph_spot_light_test) {
    for (const float cone : { 0.3F, 2.0F }) {
        BOOST_TEST_CONTEXT("cone " << cone) {
            v3d::render::realtime::canvas::Line canvas;
            v3d::editor::drawLight({ .type = v3d::editor::Light::Type::Spot, .cone = cone, .delta = 0.1F }, colour, &canvas);

            // two rings of 24 and four sides
            BOOST_CHECK_EQUAL(segments(canvas), 52U);

            // every side is one unit long, so every point but the light's own is on the unit
            // sphere about it, a cone wider than a right angle included
            for (const v3d::render::realtime::canvas::Line::Vertex& vertex : canvas.vertices()) {
                const float distance = glm::length(vertex.position);
                BOOST_CHECK(distance < 0.0001F || std::abs(distance - 1.0F) < 0.0001F);
            }
            // and the rings are at the cone's angle and its soft edge's from +z, the wider of
            // the two reaching furthest out. Past a right angle that is the inner one
            BOOST_CHECK_CLOSE(highest(canvas, 0), std::max(std::sin(cone), std::sin(cone - 0.1F)), 0.01F);
        }
    }

    // a light with no soft edge has one ring
    v3d::render::realtime::canvas::Line hard;
    v3d::editor::drawLight({ .type = v3d::editor::Light::Type::Spot, .cone = 0.5F, .delta = 0.0F }, colour, &hard);
    BOOST_CHECK_EQUAL(segments(hard), 28U);
}

BOOST_AUTO_TEST_CASE(glyph_placement_test) {
    v3d::editor::Light light;
    light.translation(glm::vec3(5.0F, 0.0F, 2.0F));
    light.rotation(glm::angleAxis(glm::radians(90.0F), glm::vec3(0.0F, 1.0F, 0.0F)));
    light.scale(glm::vec3(10.0F));

    // the translation and the rotation place the glyph, and the scale is left out, so +z one
    // unit along lands one unit away along the turned axis
    const glm::vec4 tip = v3d::editor::glyphPlacement(light.matrix()) * glm::vec4(0.0F, 0.0F, 1.0F, 1.0F);
    BOOST_CHECK_SMALL(glm::distance(glm::vec3(tip), glm::vec3(6.0F, 0.0F, 2.0F)), 0.0001F);

    // and nothing is drawn without a canvas
    v3d::editor::drawLight(light.emission(), colour, nullptr);
    v3d::editor::drawCamera(v3d::editor::Camera::Lens(), colour, nullptr);
}

};  // namespace
