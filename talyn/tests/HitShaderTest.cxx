/**
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 **/

#include <api/log/Logger.h>
#include <api/render/offline/rib/Declarations.h>
#include <api/render/offline/rib/Parameters.h>
#include <api/render/offline/sl/ShaderLibrary.h>
#include <talyn/libtalyn/HitShader.h>
#include <talyn/libtalyn/Scene.h>

#include <string>
#include <vector>

#include <boost/make_shared.hpp>
#include <boost/test/unit_test.hpp>

#include <glm/gtc/matrix_transform.hpp>
#include <glm/mat4x4.hpp>
#include <glm/vec3.hpp>

namespace {

typedef v3d::render::offline::rib::Declaration Declaration;
typedef v3d::render::offline::rib::ParameterList ParameterList;
typedef v3d::render::offline::sl::ShaderLibrary ShaderLibrary;
typedef v3d::render::offline::sl::Placed Placed;

void add(ParameterList* list, const std::string & name, Declaration::Type type,
    const std::vector<float> & values) {
    list->add(name, Declaration(Declaration::Storage::UNIFORM, type, 1), values,
        std::vector<std::string>());
}

ShaderLibrary & library() {
    static ShaderLibrary shaders(boost::make_shared<v3d::log::Logger>());
    return shaders;
}

Placed instance(const std::string & name, v3d::render::offline::sl::ShaderType type,
    const ParameterList & parameters) {
    Placed placed;
    placed.shader = library().instance(name, type, parameters);
    return placed;
}

/**
 * A triangle in the z = 0 plane wound so that its normal is along positive z, which is
 * the side the camera and the lights are on.
 **/
v3d::talyn::Triangle facing(float size, const glm::vec3 & colour) {
    return v3d::talyn::Triangle(glm::vec3(-size, -size, 0.0f), glm::vec3(size, -size, 0.0f),
        glm::vec3(0.0f, size, 0.0f), colour);
}

/**
 * The hit a ray straight down the negative z axis makes on such a triangle, at a point
 * the triangle covers.
 **/
v3d::talyn::Hit at(const v3d::talyn::Scene & scene, float x, float y) {
    const v3d::type::geometry::Ray ray(glm::vec3(x, y, 10.0f), glm::vec3(0.0f, 0.0f, -1.0f));
    v3d::talyn::Hit hit;
    BOOST_REQUIRE_MESSAGE(scene.nearest(ray, 0.0f, &hit), "the ray missed the triangle");
    return hit;
}

};  // namespace

/**
 * The done-when of the step, against hand-worked maths.
 *
 * `matte` is `Os * Cs * (Ka * ambient() + Kd * diffuse(Nf))`, and with no ambient light
 * and one distant light that is the cosine between the surface and the light. A light
 * straight overhead gives the colour back whole; one sixty degrees off gives half of it.
 **/
BOOST_AUTO_TEST_CASE(talyn_matte_at_a_hit_test) {
    v3d::talyn::Scene scene;
    v3d::talyn::Triangle triangle = facing(2.0f, glm::vec3(1.0f, 0.5f, 0.0f));
    triangle.surface(instance("matte", v3d::render::offline::sl::ShaderType::SURFACE,
        ParameterList()));
    scene.add(triangle);

    // aimed down the negative z axis, so L - which points at the light - is along positive z
    ParameterList overhead;
    add(&overhead, "to", Declaration::Type::POINT, { 0.0f, 0.0f, -1.0f });
    scene.add(instance("distantlight", v3d::render::offline::sl::ShaderType::LIGHT, overhead));

    v3d::talyn::HitShader shader(&scene);
    const glm::vec3 lit = shader.shade(at(scene, 0.0f, 0.0f));
    BOOST_CHECK_CLOSE(lit.r, 1.0f, 0.1f);
    BOOST_CHECK_CLOSE(lit.g, 0.5f, 0.1f);
    BOOST_CHECK_SMALL(lit.b, 0.0001f);
}

/**
 * Sixty degrees off the surface, whose cosine is a half. The light is tilted rather than
 * the triangle, so that the same hit is being shaded and only the lighting has changed.
 **/
BOOST_AUTO_TEST_CASE(talyn_the_cosine_test) {
    v3d::talyn::Scene scene;
    v3d::talyn::Triangle triangle = facing(2.0f, glm::vec3(1.0f));
    triangle.surface(instance("matte", v3d::render::offline::sl::ShaderType::SURFACE,
        ParameterList()));
    scene.add(triangle);

    ParameterList tilted;
    add(&tilted, "to", Declaration::Type::POINT, { 0.8660254f, 0.0f, -0.5f });
    scene.add(instance("distantlight", v3d::render::offline::sl::ShaderType::LIGHT, tilted));

    v3d::talyn::HitShader shader(&scene);
    BOOST_CHECK_CLOSE(shader.shade(at(scene, 0.0f, 0.0f)).r, 0.5f, 0.5f);
}

/**
 * An occluder darkens the point behind it. That is the whole of what `transmission` is
 * for, and it is the one thing the two renderers genuinely disagree about: moya lets all
 * the light through until it has a shadow map and talyn traces.
 **/
BOOST_AUTO_TEST_CASE(talyn_an_occluder_casts_a_shadow_test) {
    v3d::talyn::Scene scene;
    v3d::talyn::Triangle floor = facing(8.0f, glm::vec3(1.0f));
    floor.surface(instance("matte", v3d::render::offline::sl::ShaderType::SURFACE, ParameterList()));
    scene.add(floor);

    // a small triangle a unit above the floor, with no shader of its own: an occluder is
    // a visibility question and what it would have been shaded as does not come into it
    scene.add(v3d::talyn::Triangle(glm::vec3(-0.5f, -0.5f, 1.0f), glm::vec3(0.5f, -0.5f, 1.0f),
        glm::vec3(0.0f, 0.5f, 1.0f), glm::vec3(1.0f)));

    /*
        The light is up and over to one side rather than straight overhead, so the shadow
        falls beside the occluder rather than under it - a point directly beneath it is
        one the camera cannot see past the occluder to shade.
    */
    ParameterList tilted;
    add(&tilted, "to", Declaration::Type::POINT, { 0.70710678f, 0.0f, -0.70710678f });
    scene.add(instance("distantlight", v3d::render::offline::sl::ShaderType::LIGHT, tilted));

    v3d::talyn::HitShader shader(&scene);
    // one unit over from the occluder, which is where its shadow lands
    BOOST_CHECK_SMALL(shader.shade(at(scene, 1.0f, 0.0f)).r, 0.0001f);
    // and well away from it, where the cosine is all there is
    BOOST_CHECK_CLOSE(shader.shade(at(scene, 4.0f, -4.0f)).r, 0.70710678f, 0.5f);
}

/**
 * A light directly above a large polygon, which is the case the self-intersection
 * epsilon exists for.
 *
 * A shadow ray that starts exactly on the surface meets the surface it left, and every
 * lit pixel comes out black in a pattern that reads as a normal fault rather than as a
 * numerical one. The polygon is large and the point is far from its origin, because that
 * is where the arithmetic that computes the hit has the most to lose.
 **/
BOOST_AUTO_TEST_CASE(talyn_the_shadow_epsilon_test) {
    v3d::talyn::Scene scene;
    v3d::talyn::Triangle floor = facing(1000.0f, glm::vec3(1.0f));
    floor.surface(instance("matte", v3d::render::offline::sl::ShaderType::SURFACE, ParameterList()));
    scene.add(floor);

    ParameterList overhead;
    add(&overhead, "to", Declaration::Type::POINT, { 0.0f, 0.0f, -1.0f });
    scene.add(instance("distantlight", v3d::render::offline::sl::ShaderType::LIGHT, overhead));

    v3d::talyn::HitShader shader(&scene);
    BOOST_CHECK_CLOSE(shader.shade(at(scene, 0.0f, 0.0f)).r, 1.0f, 0.1f);
    BOOST_CHECK_CLOSE(shader.shade(at(scene, 400.0f, -400.0f)).r, 1.0f, 0.1f);
    BOOST_CHECK_CLOSE(shader.shade(at(scene, -700.0f, -800.0f)).r, 1.0f, 0.1f);
}

/**
 * A point light is where the scene put it, and falls off with the square of the distance
 * - which is what says its position reached the shader through the space it was
 * instanced in rather than being left at the origin.
 **/
BOOST_AUTO_TEST_CASE(talyn_a_point_light_is_placed_test) {
    v3d::talyn::Scene scene;
    v3d::talyn::Triangle floor = facing(4.0f, glm::vec3(1.0f));
    floor.surface(instance("matte", v3d::render::offline::sl::ShaderType::SURFACE, ParameterList()));
    scene.add(floor);

    Placed bulb = instance("pointlight", v3d::render::offline::sl::ShaderType::LIGHT,
        ParameterList());
    // two units above the origin of the floor, which is where the hit is
    bulb.placement = glm::translate(glm::mat4x4(1.0f), glm::vec3(0.0f, 0.0f, 2.0f));
    scene.add(bulb);

    v3d::talyn::HitShader shader(&scene);
    // the cosine is one and the falloff is a quarter, so a quarter of the colour
    BOOST_CHECK_CLOSE(shader.shade(at(scene, 0.0f, 0.0f)).r, 0.25f, 1.0f);
}

/**
 * A triangle a scene built without a shader is its own flat colour. That is the picture
 * this renderer drew before there was a language to ask for another, and it is why the
 * phase 2 reference still matches.
 **/
BOOST_AUTO_TEST_CASE(talyn_no_shader_is_the_flat_colour_test) {
    v3d::talyn::Scene scene;
    scene.add(facing(2.0f, glm::vec3(0.25f, 0.5f, 0.75f)));

    v3d::talyn::HitShader shader(&scene);
    const glm::vec3 colour = shader.shade(at(scene, 0.0f, 0.0f));
    BOOST_CHECK_CLOSE(colour.r, 0.25f, 0.1f);
    BOOST_CHECK_CLOSE(colour.b, 0.75f, 0.1f);
}
