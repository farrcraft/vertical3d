/**
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 **/

#include <api/log/Logger.h>
#include <api/render/offline/Camera.h>
#include <api/render/offline/MovingTransform.h>
#include <api/render/offline/rib/Declaration.h>
#include <api/render/offline/rib/Parameters.h>
#include <api/render/offline/sl/Placed.h>
#include <api/render/offline/sl/ShaderLibrary.h>
#include <api/render/offline/sl/Types.h>
#include <api/render/offline/trace/Hit.h>
#include <api/render/offline/trace/Scene.h>
#include <api/render/offline/trace/Sphere.h>
#include <api/render/offline/trace/Tracer.h>
#include <api/render/offline/trace/Triangle.h>
#include <api/type/geometry/Ray.h>

#include <cmath>
#include <cstdint>
#include <string>
#include <vector>

#include <boost/make_shared.hpp>
#include <boost/test/unit_test.hpp>

#include <glm/geometric.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/mat4x4.hpp>
#include <glm/vec2.hpp>
#include <glm/vec3.hpp>

namespace {

typedef v3d::render::offline::rib::Declaration Declaration;
typedef v3d::render::offline::rib::ParameterList ParameterList;
typedef v3d::render::offline::trace::Scene Scene;

/** What the derivatives fixture writes into Ci. **/
enum class Chosen : std::uint8_t { DU_P, DV_P, NORMAL, STEPS };

/**
 * The derivatives fixture, writing one of its answers. It is a shader on the suite's search
 * path rather than a standard one.
 **/
v3d::render::offline::sl::Placed derivatives(Chosen which) {
    static v3d::render::offline::sl::ShaderLibrary shaders(boost::make_shared<v3d::log::Logger>());
    shaders.searchpath("data");
    ParameterList parameters;
    parameters.add("which", Declaration(Declaration::Storage::UNIFORM, Declaration::Type::FLOAT, 1),
        { static_cast<float>(which) }, std::vector<std::string>());
    v3d::render::offline::sl::Placed placed;
    placed.shader = shaders.instance("derivatives", v3d::render::offline::sl::ShaderType::SURFACE, parameters);
    BOOST_REQUIRE(placed.shader);
    return placed;
}

/**
 * A camera whose raster space is world space scaled up, so a pixel is 1 / scale wide
 * everywhere, as under an orthographic camera.
 **/
void camera(Scene* scene, float scale) {
    v3d::render::offline::Camera camera;
    camera.raster = glm::scale(glm::mat4x4(1.0F), glm::vec3(scale));
    scene->camera(camera);
}

/** The hit a ray down onto a point makes, coming from a few units along a direction. **/
v3d::render::offline::trace::Hit at(const Scene & scene, const glm::vec3 & point, const glm::vec3 & from) {
    const v3d::type::geometry::Ray ray({.origin = point + (from * 5.0F), .direction = -from});
    v3d::render::offline::trace::Hit hit;
    BOOST_REQUIRE_MESSAGE(scene.nearest(ray, 0.0F, &hit), "the ray missed");
    return hit;
}

/** The edge of a tilted triangle from its first corner to its second. **/
glm::vec3 edgeU() {
    return {2.0F, 0.5F, 1.0F};
}

/** The same triangle's edge from its first corner to its third. **/
glm::vec3 edgeV() {
    return {-0.5F, 3.0F, 0.25F};
}

/**
 * A scene of one tilted triangle with corners a, a + edgeU() and a + edgeV(), so its
 * parameters are those of a grid over the same plane, shaded by the derivatives fixture.
 **/
Scene triangle(Chosen which, float scale) {
    Scene scene;
    const glm::vec3 corner(1.0F, 2.0F, 0.0F);
    v3d::render::offline::trace::Triangle tilted({corner, corner + edgeU(), corner + edgeV()}, glm::vec3(1.0F));
    tilted.surface(derivatives(which));
    scene.add(tilted);
    camera(&scene, scale);
    return scene;
}

/** Where on the triangle a case looks, and how many pixels a unit its camera has. **/
struct Look {
    float pixels = 1.0F;
    /** How far along each edge the point is. **/
    float along = 0.25F;
};

/** What the fixture writes at a point on the triangle the same way along each edge. **/
glm::vec3 shadeTriangle(Chosen which, const Look & look) {
    const Scene scene = triangle(which, look.pixels);
    const glm::vec3 normal = glm::normalize(glm::cross(edgeU(), edgeV()));
    const glm::vec3 point = glm::vec3(1.0F, 2.0F, 0.0F) + (look.along * edgeU()) + (look.along * edgeV());
    v3d::render::offline::trace::Tracer tracer(&scene);
    return tracer.shade(at(scene, point, normal));
}

/**
 * A traced hit has derivatives. On a triangle over a plane, Du(P) and Dv(P) are its edges,
 * as they are on a grid over the same plane, and calculatenormal(P) is the plane's normal.
 **/
BOOST_AUTO_TEST_CASE(trace_a_hit_has_derivatives_over_a_plane_test) {
    BOOST_TEST(glm::length(shadeTriangle(Chosen::DU_P, {.pixels = 200.0F, .along = 0.25F}) - edgeU()) < 1.0e-3F);
    BOOST_TEST(glm::length(shadeTriangle(Chosen::DV_P, {.pixels = 200.0F, .along = 0.25F}) - edgeV()) < 1.0e-3F);
    const glm::vec3 normal = glm::cross(edgeU(), edgeV());
    BOOST_TEST(glm::length(shadeTriangle(Chosen::NORMAL, {.pixels = 200.0F, .along = 0.25F}) - normal) < 1.0e-2F);
}

/**
 * du and dv are the stencil's steps: the parameter change that moves the surface one pixel.
 * At 200 pixels a unit, that is 1 / 200 over the length of each edge.
 **/
BOOST_AUTO_TEST_CASE(trace_du_and_dv_are_a_pixel_test) {
    const glm::vec3 steps = shadeTriangle(Chosen::STEPS, {.pixels = 200.0F, .along = 0.25F});
    BOOST_CHECK_CLOSE(steps.x, 0.005F / glm::length(edgeU()), 0.5F);
    BOOST_CHECK_CLOSE(steps.y, 0.005F / glm::length(edgeV()), 0.5F);
}

/**
 * A stencil near a triangle's edge reaches past the edge, onto the triangle's plane carried
 * on. Its normal is still the plane's. At two pixels a unit the stencil is half a unit across,
 * and a hit a hundredth inside the far edge has its neighbours well outside it.
 **/
BOOST_AUTO_TEST_CASE(trace_a_stencil_past_an_edge_stays_on_the_plane_test) {
    const glm::vec3 normal = glm::cross(edgeU(), edgeV());
    BOOST_TEST(glm::length(shadeTriangle(Chosen::NORMAL, {.pixels = 2.0F, .along = 0.495F}) - normal) < 1.0e-2F);

    const Scene scene = triangle(Chosen::NORMAL, 2.0F);
    const glm::vec3 point = glm::vec3(1.0F, 2.0F, 0.0F) + (0.495F * edgeU()) + (0.495F * edgeV());
    const v3d::render::offline::trace::Hit hit = at(scene, point, glm::normalize(normal));
    v3d::render::offline::trace::Hit beyond;
    BOOST_REQUIRE(Scene::around(hit, glm::vec2(hit.u + 0.5F, hit.v), scene.poses(0.0F), &beyond));
    BOOST_CHECK_GT(beyond.u + beyond.v, 1.0F);
    BOOST_CHECK_SMALL(glm::dot(beyond.point - hit.point, glm::normalize(normal)), 1.0e-4F);
}

/**
 * On a curved surface the stencil's difference is close to the true derivative when a pixel
 * is small. A unit sphere's u sweeps the whole turn and its v runs from pole to pole. At
 * longitude phi and latitude theta, dP/du is 2 pi (-cos theta sin phi, cos theta cos phi, 0).
 **/
BOOST_AUTO_TEST_CASE(trace_a_hit_has_derivatives_on_a_sphere_test) {
    const float pi = 3.14159265F;
    const float phi = 0.6F;
    const float theta = 0.3F;
    const glm::vec3 point(std::cos(theta) * std::cos(phi), std::cos(theta) * std::sin(phi), std::sin(theta));
    for (const Chosen which : { Chosen::DU_P, Chosen::DV_P }) {
        Scene scene;
        v3d::render::offline::trace::Sphere ball({.radius = 1.0F, .zmin = -1.0F, .zmax = 1.0F, .thetamax = 360.0F},
            glm::mat4x4(1.0F), glm::vec3(1.0F));
        ball.surface(derivatives(which));
        scene.add(ball, v3d::render::offline::MovingTransform());
        camera(&scene, 1000.0F);
        v3d::render::offline::trace::Tracer tracer(&scene);
        const glm::vec3 shaded = tracer.shade(at(scene, point, point));
        const glm::vec3 expected = which == Chosen::DU_P ?
            2.0F * pi * glm::vec3(-std::cos(theta) * std::sin(phi), std::cos(theta) * std::cos(phi), 0.0F) :
            pi * glm::vec3(-std::sin(theta) * std::cos(phi), -std::sin(theta) * std::sin(phi), std::cos(theta));
        BOOST_TEST_CONTEXT((which == Chosen::DU_P ? "Du" : "Dv")) {
            BOOST_TEST(glm::length(shaded - expected) < 0.01F * glm::length(expected));
        }
    }
}

/**
 * A scene with no camera has a pixel one unit wide, through the identity projection.
 **/
BOOST_AUTO_TEST_CASE(trace_a_footprint_without_a_camera_test) {
    const Scene scene;
    BOOST_CHECK_CLOSE(scene.footprint(glm::vec3(3.0F, -2.0F, 7.0F)), 1.0F, 0.01F);
    Scene scaled;
    camera(&scaled, 50.0F);
    BOOST_CHECK_CLOSE(scaled.footprint(glm::vec3(3.0F, -2.0F, 7.0F)), 0.02F, 0.01F);
}

};  // namespace
