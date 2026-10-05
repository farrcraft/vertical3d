/**
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 **/

#include <api/log/Logger.h>
#include <api/render/offline/rib/Declaration.h>
#include <api/render/offline/rib/Parameters.h>
#include <api/render/offline/sl/Placed.h>
#include <api/render/offline/sl/ShaderLibrary.h>
#include <api/render/offline/trace/Hit.h>
#include <api/render/offline/trace/Tracer.h>
#include <api/render/offline/trace/Scene.h>
#include <api/render/offline/trace/Triangle.h>

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
v3d::render::offline::trace::Triangle facing(float size, const glm::vec3 & colour) {
    return v3d::render::offline::trace::Triangle(glm::vec3(-size, -size, 0.0f), glm::vec3(size, -size, 0.0f),
        glm::vec3(0.0f, size, 0.0f), colour);
}

/**
 * The hit a ray straight down the negative z axis makes on such a triangle, at a point
 * the triangle covers.
 **/
v3d::render::offline::trace::Hit at(const v3d::render::offline::trace::Scene & scene, float x, float y) {
    const v3d::type::geometry::Ray ray(glm::vec3(x, y, 10.0f), glm::vec3(0.0f, 0.0f, -1.0f));
    v3d::render::offline::trace::Hit hit;
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
BOOST_AUTO_TEST_CASE(trace_matte_at_a_hit_test) {
    v3d::render::offline::trace::Scene scene;
    v3d::render::offline::trace::Triangle triangle = facing(2.0f, glm::vec3(1.0f, 0.5f, 0.0f));
    triangle.surface(instance("matte", v3d::render::offline::sl::ShaderType::SURFACE,
        ParameterList()));
    scene.add(triangle);

    // aimed down the negative z axis, so L - which points at the light - is along positive z
    ParameterList overhead;
    add(&overhead, "to", Declaration::Type::POINT, { 0.0f, 0.0f, -1.0f });
    scene.add(instance("distantlight", v3d::render::offline::sl::ShaderType::LIGHT, overhead));

    v3d::render::offline::trace::Tracer shader(&scene);
    const glm::vec3 lit = shader.shade(at(scene, 0.0f, 0.0f));
    BOOST_CHECK_CLOSE(lit.r, 1.0f, 0.1f);
    BOOST_CHECK_CLOSE(lit.g, 0.5f, 0.1f);
    BOOST_CHECK_SMALL(lit.b, 0.0001f);
}

/**
 * Sixty degrees off the surface, whose cosine is a half. The light is tilted rather than
 * the triangle, so that the same hit is being shaded and only the lighting has changed.
 **/
BOOST_AUTO_TEST_CASE(trace_the_cosine_test) {
    v3d::render::offline::trace::Scene scene;
    v3d::render::offline::trace::Triangle triangle = facing(2.0f, glm::vec3(1.0f));
    triangle.surface(instance("matte", v3d::render::offline::sl::ShaderType::SURFACE,
        ParameterList()));
    scene.add(triangle);

    ParameterList tilted;
    add(&tilted, "to", Declaration::Type::POINT, { 0.8660254f, 0.0f, -0.5f });
    scene.add(instance("distantlight", v3d::render::offline::sl::ShaderType::LIGHT, tilted));

    v3d::render::offline::trace::Tracer shader(&scene);
    BOOST_CHECK_CLOSE(shader.shade(at(scene, 0.0f, 0.0f)).r, 0.5f, 0.5f);
}

/**
 * An occluder darkens the point behind it. That is the whole of what `transmission` is
 * for, and the tracer answers it by casting a ray to the light.
 **/
BOOST_AUTO_TEST_CASE(trace_an_occluder_casts_a_shadow_test) {
    v3d::render::offline::trace::Scene scene;
    v3d::render::offline::trace::Triangle floor = facing(8.0f, glm::vec3(1.0f));
    floor.surface(instance("matte", v3d::render::offline::sl::ShaderType::SURFACE, ParameterList()));
    scene.add(floor);

    // a small triangle a unit above the floor, with no shader of its own: an occluder is
    // a visibility question and what it would have been shaded as does not come into it
    scene.add(v3d::render::offline::trace::Triangle(glm::vec3(-0.5f, -0.5f, 1.0f), glm::vec3(0.5f, -0.5f, 1.0f),
        glm::vec3(0.0f, 0.5f, 1.0f), glm::vec3(1.0f)));

    /*
        The light is up and over to one side rather than straight overhead, so the shadow
        falls beside the occluder rather than under it - a point directly beneath it is
        one the camera cannot see past the occluder to shade.
    */
    ParameterList tilted;
    add(&tilted, "to", Declaration::Type::POINT, { 0.70710678f, 0.0f, -0.70710678f });
    scene.add(instance("distantlight", v3d::render::offline::sl::ShaderType::LIGHT, tilted));

    v3d::render::offline::trace::Tracer shader(&scene);
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
BOOST_AUTO_TEST_CASE(trace_the_shadow_epsilon_test) {
    v3d::render::offline::trace::Scene scene;
    v3d::render::offline::trace::Triangle floor = facing(1000.0f, glm::vec3(1.0f));
    floor.surface(instance("matte", v3d::render::offline::sl::ShaderType::SURFACE, ParameterList()));
    scene.add(floor);

    ParameterList overhead;
    add(&overhead, "to", Declaration::Type::POINT, { 0.0f, 0.0f, -1.0f });
    scene.add(instance("distantlight", v3d::render::offline::sl::ShaderType::LIGHT, overhead));

    v3d::render::offline::trace::Tracer shader(&scene);
    BOOST_CHECK_CLOSE(shader.shade(at(scene, 0.0f, 0.0f)).r, 1.0f, 0.1f);
    BOOST_CHECK_CLOSE(shader.shade(at(scene, 400.0f, -400.0f)).r, 1.0f, 0.1f);
    BOOST_CHECK_CLOSE(shader.shade(at(scene, -700.0f, -800.0f)).r, 1.0f, 0.1f);
}

/**
 * A point light is where the scene put it, and falls off with the square of the distance
 * - which is what says its position reached the shader through the space it was
 * instanced in rather than being left at the origin.
 **/
BOOST_AUTO_TEST_CASE(trace_a_point_light_is_placed_test) {
    v3d::render::offline::trace::Scene scene;
    v3d::render::offline::trace::Triangle floor = facing(4.0f, glm::vec3(1.0f));
    floor.surface(instance("matte", v3d::render::offline::sl::ShaderType::SURFACE, ParameterList()));
    scene.add(floor);

    Placed bulb = instance("pointlight", v3d::render::offline::sl::ShaderType::LIGHT,
        ParameterList());
    // two units above the origin of the floor, which is where the hit is
    bulb.placement = glm::translate(glm::mat4x4(1.0f), glm::vec3(0.0f, 0.0f, 2.0f));
    scene.add(bulb);

    v3d::render::offline::trace::Tracer shader(&scene);
    // the cosine is one and the falloff is a quarter, so a quarter of the colour
    BOOST_CHECK_CLOSE(shader.shade(at(scene, 0.0f, 0.0f)).r, 0.25f, 1.0f);
}

/**
 * A triangle a scene built without a shader is its own flat colour. That is the picture
 * this renderer drew before there was a language to ask for another, and it is why the
 * phase 2 reference still matches.
 **/
BOOST_AUTO_TEST_CASE(trace_no_shader_is_the_flat_colour_test) {
    v3d::render::offline::trace::Scene scene;
    scene.add(facing(2.0f, glm::vec3(0.25f, 0.5f, 0.75f)));

    v3d::render::offline::trace::Tracer shader(&scene);
    const glm::vec3 colour = shader.shade(at(scene, 0.0f, 0.0f));
    BOOST_CHECK_CLOSE(colour.r, 0.25f, 0.1f);
    BOOST_CHECK_CLOSE(colour.b, 0.75f, 0.1f);
}

namespace {

/**
 * Two triangles a unit apart facing each other, each shaded by `relay`, which adds its own
 * colour to what it traces along its normal: red below, facing up, and green above, facing
 * down. The hit is on the red one, seen from between them.
 **/
glm::vec3 relayed(unsigned int depth) {
    static ShaderLibrary shaders(boost::make_shared<v3d::log::Logger>());
    shaders.searchpath("data");

    v3d::render::offline::trace::Scene scene;
    scene.traceDepth(depth);
    v3d::render::offline::trace::Triangle below = facing(2.0f, glm::vec3(1.0f, 0.0f, 0.0f));
    v3d::render::offline::trace::Triangle above(glm::vec3(-2.0f, -2.0f, 1.0f), glm::vec3(0.0f, 2.0f, 1.0f),
        glm::vec3(2.0f, -2.0f, 1.0f), glm::vec3(0.0f, 1.0f, 0.0f));
    Placed relay;
    relay.shader = shaders.instance("relay", v3d::render::offline::sl::ShaderType::SURFACE,
        ParameterList());
    BOOST_REQUIRE(relay.shader);
    below.surface(relay);
    above.surface(relay);
    scene.add(below);
    scene.add(above);

    const v3d::type::geometry::Ray ray(glm::vec3(0.0f, 0.0f, 0.5f), glm::vec3(0.0f, 0.0f, -1.0f));
    v3d::render::offline::trace::Hit hit;
    BOOST_REQUIRE(scene.nearest(ray, 0.0f, &hit));
    v3d::render::offline::trace::Tracer shader(&scene);
    return shader.shade(hit);
}

};  // namespace

/**
 * A surface tracing into a surface with the same shader gets that surface's colour, and
 * its own registers survive the trace.
 *
 * At a depth of two the red surface sees the green one, which sees the red one, which
 * traces no further: red, plus green, plus red. A machine shared between the levels would
 * have the outer red read the inner Cs after its trace returned, and the sum would not be
 * this one.
 **/
BOOST_AUTO_TEST_CASE(trace_a_trace_recurses_test) {
    const glm::vec3 colour = relayed(2);
    BOOST_CHECK_CLOSE(colour.r, 2.0f, 0.1f);
    BOOST_CHECK_CLOSE(colour.g, 1.0f, 0.1f);
    BOOST_CHECK_SMALL(colour.b, 0.0001f);
}

/**
 * The scene's depth is where the trace stops, and past it a trace answers the background,
 * which is black here.
 **/
BOOST_AUTO_TEST_CASE(trace_a_trace_stops_at_the_depth_test) {
    const glm::vec3 once = relayed(1);
    BOOST_CHECK_CLOSE(once.r, 1.0f, 0.1f);
    BOOST_CHECK_CLOSE(once.g, 1.0f, 0.1f);

    const glm::vec3 never = relayed(0);
    BOOST_CHECK_CLOSE(never.r, 1.0f, 0.1f);
    BOOST_CHECK_SMALL(never.g, 0.0001f);

    const glm::vec3 deeper = relayed(3);
    BOOST_CHECK_CLOSE(deeper.r, 2.0f, 0.1f);
    BOOST_CHECK_CLOSE(deeper.g, 2.0f, 0.1f);
}

namespace {

/** What a ray straight down the negative z axis from a height sees. **/
v3d::render::offline::trace::Tracer::Seen down(v3d::render::offline::trace::Tracer* shader, float x, float y, float from) {
    return shader->see(v3d::type::geometry::Ray(glm::vec3(x, y, from), glm::vec3(0.0f, 0.0f, -1.0f)));
}

/** A triangle in the plane z = height, facing down the negative z axis. **/
v3d::render::offline::trace::Triangle downward(float size, float height, const glm::vec3 & colour) {
    return v3d::render::offline::trace::Triangle(glm::vec3(-size, -size, height), glm::vec3(0.0f, size, height),
        glm::vec3(size, -size, height), colour);
}

};  // namespace

/**
 * A mirror facing a red quad shows the red quad, exactly: shinymetal with its ambient and its
 * highlight off is Cs times what it traces, and the quad has no shader of its own.
 **/
BOOST_AUTO_TEST_CASE(trace_a_mirror_shows_what_it_faces_test) {
    v3d::render::offline::trace::Scene scene;
    v3d::render::offline::trace::Triangle mirror = facing(2.0f, glm::vec3(1.0f));
    ParameterList only;
    add(&only, "Ka", Declaration::Type::FLOAT, { 0.0f });
    add(&only, "Ks", Declaration::Type::FLOAT, { 0.0f });
    mirror.surface(instance("shinymetal", v3d::render::offline::sl::ShaderType::SURFACE, only));
    scene.add(mirror);
    scene.add(downward(4.0f, 2.0f, glm::vec3(1.0f, 0.0f, 0.0f)));

    v3d::render::offline::trace::Tracer shader(&scene);
    const glm::vec3 seen = down(&shader, 0.0f, 0.0f, 1.0f).colour;
    BOOST_CHECK_CLOSE(seen.r, 1.0f, 0.001f);
    BOOST_CHECK_SMALL(seen.g, 1.0e-6f);
    BOOST_CHECK_SMALL(seen.b, 1.0e-6f);
}

/**
 * A half opaque white quad over a black one composites to grey through its Oi, and over
 * nothing it lets half the background through.
 **/
BOOST_AUTO_TEST_CASE(trace_transparency_composites_test) {
    const Placed constant = instance("constant", v3d::render::offline::sl::ShaderType::SURFACE, ParameterList());
    v3d::render::offline::trace::Scene scene;
    scene.background(glm::vec3(0.0f, 0.0f, 1.0f));
    v3d::render::offline::trace::Triangle pane(glm::vec3(-4.0f, -4.0f, 1.0f), glm::vec3(4.0f, -4.0f, 1.0f),
        glm::vec3(0.0f, 4.0f, 1.0f), glm::vec3(1.0f));
    pane.surface(constant);
    pane.opacity(glm::vec3(0.5f));
    scene.add(pane);
    // the black quad is under the left of the pane only
    v3d::render::offline::trace::Triangle under(glm::vec3(-3.0f, -3.0f, 0.0f), glm::vec3(-1.0f, -3.0f, 0.0f),
        glm::vec3(-1.0f, 3.0f, 0.0f), glm::vec3(0.0f));
    under.surface(constant);
    scene.add(under);

    v3d::render::offline::trace::Tracer shader(&scene);
    const v3d::render::offline::trace::Tracer::Seen over = down(&shader, -1.5f, -1.0f, 10.0f);
    BOOST_CHECK(over.hit);
    BOOST_CHECK_CLOSE(over.colour.r, 0.5f, 0.001f);
    BOOST_CHECK_CLOSE(over.colour.b, 0.5f, 0.001f);
    BOOST_CHECK_CLOSE(over.opacity.r, 1.0f, 0.001f);
    BOOST_CHECK_CLOSE(over.distance, 9.0f, 0.001f);

    const v3d::render::offline::trace::Tracer::Seen alone = down(&shader, 1.0f, -1.0f, 10.0f);
    BOOST_CHECK_CLOSE(alone.colour.r, 0.5f, 0.001f);
    BOOST_CHECK_CLOSE(alone.colour.b, 1.0f, 0.001f);
    BOOST_CHECK_CLOSE(alone.opacity.r, 0.5f, 0.001f);
}

/**
 * A shadow through a half opaque occluder is half as dark: the occluder test's scene with
 * the occluder's Os at a half, where its shadow lands.
 **/
BOOST_AUTO_TEST_CASE(trace_a_shadow_through_a_pane_test) {
    v3d::render::offline::trace::Scene scene;
    v3d::render::offline::trace::Triangle floor = facing(8.0f, glm::vec3(1.0f));
    floor.surface(instance("matte", v3d::render::offline::sl::ShaderType::SURFACE, ParameterList()));
    scene.add(floor);
    v3d::render::offline::trace::Triangle pane(glm::vec3(-0.5f, -0.5f, 1.0f), glm::vec3(0.5f, -0.5f, 1.0f),
        glm::vec3(0.0f, 0.5f, 1.0f), glm::vec3(1.0f));
    pane.opacity(glm::vec3(0.5f));
    scene.add(pane);

    ParameterList tilted;
    add(&tilted, "to", Declaration::Type::POINT, { 0.70710678f, 0.0f, -0.70710678f });
    scene.add(instance("distantlight", v3d::render::offline::sl::ShaderType::LIGHT, tilted));

    v3d::render::offline::trace::Tracer shader(&scene);
    BOOST_CHECK_CLOSE(shader.shade(at(scene, 1.0f, 0.0f)).r, 0.5f * 0.70710678f, 0.5f);
}

/**
 * A ray through a glass slab at normal incidence comes out where it went in: straight on,
 * with what each face reflects taken off. Each face passes 0.96, the fresnel transmittance of
 * glass of index 1.5 straight on, so the red quad under the slab is 0.9216 of itself and the
 * floor beside it is still black.
 **/
BOOST_AUTO_TEST_CASE(trace_a_glass_slab_is_straight_through_test) {
    const Placed glass = instance("glass", v3d::render::offline::sl::ShaderType::SURFACE, ParameterList());
    v3d::render::offline::trace::Scene scene;
    // the slab's two faces, each facing out of it
    v3d::render::offline::trace::Triangle top(glm::vec3(-4.0f, -4.0f, 1.0f), glm::vec3(4.0f, -4.0f, 1.0f),
        glm::vec3(0.0f, 4.0f, 1.0f), glm::vec3(1.0f));
    top.surface(glass);
    scene.add(top);
    v3d::render::offline::trace::Triangle bottom = downward(4.0f, 0.5f, glm::vec3(1.0f));
    bottom.surface(glass);
    scene.add(bottom);
    scene.add(v3d::render::offline::trace::Triangle(glm::vec3(-0.5f, -0.5f, 0.0f), glm::vec3(0.5f, -0.5f, 0.0f),
        glm::vec3(0.0f, 0.5f, 0.0f), glm::vec3(1.0f, 0.0f, 0.0f)));

    v3d::render::offline::trace::Tracer shader(&scene);
    const glm::vec3 through = down(&shader, 0.0f, 0.0f, 10.0f).colour;
    BOOST_CHECK_CLOSE(through.r, 0.96f * 0.96f, 0.01f);
    BOOST_CHECK_SMALL(through.g, 1.0e-6f);
    BOOST_CHECK_SMALL(down(&shader, 0.0f, -0.7f, 10.0f).colour.r, 1.0e-6f);
}

/**
 * A primitive given lights of its own is shaded by those rather than by the scene's, per
 * ADR-0077: the scene's list is for a primitive that was given none.
 **/
BOOST_AUTO_TEST_CASE(trace_a_primitive_has_its_own_lights_test) {
    v3d::render::offline::trace::Scene scene;
    v3d::render::offline::trace::Triangle triangle = facing(2.0f, glm::vec3(1.0f));
    triangle.surface(instance("matte", v3d::render::offline::sl::ShaderType::SURFACE,
        ParameterList()));

    // the scene's light is overhead and the primitive's own is sixty degrees off
    ParameterList overhead;
    add(&overhead, "to", Declaration::Type::POINT, { 0.0f, 0.0f, -1.0f });
    scene.add(instance("distantlight", v3d::render::offline::sl::ShaderType::LIGHT, overhead));
    ParameterList tilted;
    add(&tilted, "to", Declaration::Type::POINT, { 0.8660254f, 0.0f, -0.5f });
    triangle.lights(boost::make_shared<std::vector<Placed> >(
        1, instance("distantlight", v3d::render::offline::sl::ShaderType::LIGHT, tilted)));
    scene.add(triangle);

    v3d::render::offline::trace::Tracer shader(&scene);
    BOOST_CHECK_CLOSE(shader.shade(at(scene, 0.0f, 0.0f)).r, 0.5f, 0.5f);
}
