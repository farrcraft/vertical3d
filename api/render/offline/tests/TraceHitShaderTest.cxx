/**
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 **/

#include <api/log/Logger.h>
#include <api/render/offline/rib/Declaration.h>
#include <api/render/offline/rib/Parameters.h>
#include <api/render/offline/sl/Placed.h>
#include <api/render/offline/sl/ShaderLibrary.h>
#include <api/render/offline/sl/Types.h>
#include <api/render/offline/trace/Hit.h>
#include <api/render/offline/trace/HitShader.h>
#include <api/render/offline/trace/Tracer.h>
#include <api/render/offline/trace/Scene.h>
#include <api/render/offline/trace/Triangle.h>
#include <api/type/geometry/Ray.h>

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
    return v3d::render::offline::trace::Triangle({glm::vec3(-size, -size, 0.0F), glm::vec3(size, -size, 0.0F), glm::vec3(0.0F, size, 0.0F)}, colour);
}

/**
 * The hit a ray straight down the negative z axis makes on such a triangle, at a point
 * the triangle covers.
 **/
v3d::render::offline::trace::Hit at(const v3d::render::offline::trace::Scene & scene, float x, float y) {
    const v3d::type::geometry::Ray ray({.origin = glm::vec3(x, y, 10.0F), .direction = glm::vec3(0.0F, 0.0F, -1.0F)});
    v3d::render::offline::trace::Hit hit;
    BOOST_REQUIRE_MESSAGE(scene.nearest(ray, 0.0F, &hit), "the ray missed the triangle");
    return hit;
}

/**
 * `matte` over one distant light, against hand-worked maths.
 *
 * `matte` is `Os * Cs * (Ka * ambient() + Kd * diffuse(Nf))`, and with no ambient light
 * and one distant light that is the cosine between the surface and the light. A light
 * straight overhead gives the colour back whole; one sixty degrees off gives half of it.
 **/
BOOST_AUTO_TEST_CASE(trace_matte_at_a_hit_test) {
    v3d::render::offline::trace::Scene scene;
    v3d::render::offline::trace::Triangle triangle = facing(2.0F, glm::vec3(1.0F, 0.5F, 0.0F));
    triangle.surface(instance("matte", v3d::render::offline::sl::ShaderType::SURFACE,
        ParameterList()));
    scene.add(triangle);

    // aimed down the negative z axis, so L, which points at the light, is along positive z
    ParameterList overhead;
    add(&overhead, "to", Declaration::Type::POINT, { 0.0F, 0.0F, -1.0F });
    scene.add(instance("distantlight", v3d::render::offline::sl::ShaderType::LIGHT, overhead));

    v3d::render::offline::trace::Tracer shader(&scene);
    const glm::vec3 lit = shader.shade(at(scene, 0.0F, 0.0F));
    BOOST_CHECK_CLOSE(lit.r, 1.0F, 0.1F);
    BOOST_CHECK_CLOSE(lit.g, 0.5F, 0.1F);
    BOOST_CHECK_SMALL(lit.b, 0.0001F);
}

/**
 * Sixty degrees off the surface, whose cosine is a half. The light is tilted rather than
 * the triangle, so that the same hit is being shaded and only the lighting has changed.
 **/
BOOST_AUTO_TEST_CASE(trace_the_cosine_test) {
    v3d::render::offline::trace::Scene scene;
    v3d::render::offline::trace::Triangle triangle = facing(2.0F, glm::vec3(1.0F));
    triangle.surface(instance("matte", v3d::render::offline::sl::ShaderType::SURFACE,
        ParameterList()));
    scene.add(triangle);

    ParameterList tilted;
    add(&tilted, "to", Declaration::Type::POINT, { 0.8660254F, 0.0F, -0.5F });
    scene.add(instance("distantlight", v3d::render::offline::sl::ShaderType::LIGHT, tilted));

    v3d::render::offline::trace::Tracer shader(&scene);
    BOOST_CHECK_CLOSE(shader.shade(at(scene, 0.0F, 0.0F)).r, 0.5F, 0.5F);
}

/**
 * An occluder darkens the point behind it. The tracer computes `transmission` by casting a
 * ray to the light.
 **/
BOOST_AUTO_TEST_CASE(trace_an_occluder_casts_a_shadow_test) {
    v3d::render::offline::trace::Scene scene;
    v3d::render::offline::trace::Triangle floor = facing(8.0F, glm::vec3(1.0F));
    floor.surface(instance("matte", v3d::render::offline::sl::ShaderType::SURFACE, ParameterList()));
    scene.add(floor);

    // a small triangle a unit above the floor, with no shader of its own: a shadow ray
    // reads an occluder's opacity and does not run its shader
    scene.add(v3d::render::offline::trace::Triangle({glm::vec3(-0.5F, -0.5F, 1.0F), glm::vec3(0.5F, -0.5F, 1.0F), glm::vec3(0.0F, 0.5F, 1.0F)}, glm::vec3(1.0F)));

    /*
        The light is up and over to one side, not straight overhead. The shadow falls
        beside the occluder, not under it. The camera cannot see a point directly beneath
        the occluder.
    */
    ParameterList tilted;
    add(&tilted, "to", Declaration::Type::POINT, { 0.70710678F, 0.0F, -0.70710678F });
    scene.add(instance("distantlight", v3d::render::offline::sl::ShaderType::LIGHT, tilted));

    v3d::render::offline::trace::Tracer shader(&scene);
    // one unit over from the occluder, where its shadow lands
    BOOST_CHECK_SMALL(shader.shade(at(scene, 1.0F, 0.0F)).r, 0.0001F);
    // well away from it, where only the cosine applies
    BOOST_CHECK_CLOSE(shader.shade(at(scene, 4.0F, -4.0F)).r, 0.70710678F, 0.5F);
}

/**
 * A light directly above a large polygon, the case the self-intersection offset guards
 * against.
 *
 * A shadow ray that starts exactly on the surface meets the surface it left, and every
 * lit pixel comes out black. The polygon is large and the point is far from its origin,
 * because that is where the hit computation loses the most precision.
 **/
BOOST_AUTO_TEST_CASE(trace_the_shadow_epsilon_test) {
    v3d::render::offline::trace::Scene scene;
    v3d::render::offline::trace::Triangle floor = facing(1000.0F, glm::vec3(1.0F));
    floor.surface(instance("matte", v3d::render::offline::sl::ShaderType::SURFACE, ParameterList()));
    scene.add(floor);

    ParameterList overhead;
    add(&overhead, "to", Declaration::Type::POINT, { 0.0F, 0.0F, -1.0F });
    scene.add(instance("distantlight", v3d::render::offline::sl::ShaderType::LIGHT, overhead));

    v3d::render::offline::trace::Tracer shader(&scene);
    BOOST_CHECK_CLOSE(shader.shade(at(scene, 0.0F, 0.0F)).r, 1.0F, 0.1F);
    BOOST_CHECK_CLOSE(shader.shade(at(scene, 400.0F, -400.0F)).r, 1.0F, 0.1F);
    BOOST_CHECK_CLOSE(shader.shade(at(scene, -700.0F, -800.0F)).r, 1.0F, 0.1F);
}

/**
 * A point light is where the scene put it, and falls off with the square of the distance.
 * This shows its position reached the shader through the space it was instanced in rather
 * than being left at the origin.
 **/
BOOST_AUTO_TEST_CASE(trace_a_point_light_is_placed_test) {
    v3d::render::offline::trace::Scene scene;
    v3d::render::offline::trace::Triangle floor = facing(4.0F, glm::vec3(1.0F));
    floor.surface(instance("matte", v3d::render::offline::sl::ShaderType::SURFACE, ParameterList()));
    scene.add(floor);

    Placed bulb = instance("pointlight", v3d::render::offline::sl::ShaderType::LIGHT,
        ParameterList());
    // two units above the origin of the floor, where the hit is
    bulb.placement = glm::translate(glm::mat4x4(1.0F), glm::vec3(0.0F, 0.0F, 2.0F));
    scene.add(bulb);

    v3d::render::offline::trace::Tracer shader(&scene);
    // the cosine is one and the falloff is a quarter, so a quarter of the colour
    BOOST_CHECK_CLOSE(shader.shade(at(scene, 0.0F, 0.0F)).r, 0.25F, 1.0F);
}

/**
 * A triangle a scene built without a shader is drawn in its own flat colour.
 **/
BOOST_AUTO_TEST_CASE(trace_no_shader_is_the_flat_colour_test) {
    v3d::render::offline::trace::Scene scene;
    scene.add(facing(2.0F, glm::vec3(0.25F, 0.5F, 0.75F)));

    v3d::render::offline::trace::Tracer shader(&scene);
    const glm::vec3 colour = shader.shade(at(scene, 0.0F, 0.0F));
    BOOST_CHECK_CLOSE(colour.r, 0.25F, 0.1F);
    BOOST_CHECK_CLOSE(colour.b, 0.75F, 0.1F);
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
    v3d::render::offline::trace::Triangle below = facing(2.0F, glm::vec3(1.0F, 0.0F, 0.0F));
    v3d::render::offline::trace::Triangle above({glm::vec3(-2.0F, -2.0F, 1.0F), glm::vec3(0.0F, 2.0F, 1.0F), glm::vec3(2.0F, -2.0F, 1.0F)}, glm::vec3(0.0F, 1.0F, 0.0F));
    Placed relay;
    relay.shader = shaders.instance("relay", v3d::render::offline::sl::ShaderType::SURFACE,
        ParameterList());
    BOOST_REQUIRE(relay.shader);
    below.surface(relay);
    above.surface(relay);
    scene.add(below);
    scene.add(above);

    const v3d::type::geometry::Ray ray({.origin = glm::vec3(0.0F, 0.0F, 0.5F), .direction = glm::vec3(0.0F, 0.0F, -1.0F)});
    v3d::render::offline::trace::Hit hit;
    BOOST_REQUIRE(scene.nearest(ray, 0.0F, &hit));
    v3d::render::offline::trace::Tracer shader(&scene);
    return shader.shade(hit);
}

};  // namespace

/**
 * A surface tracing into a surface with the same shader gets that surface's colour, and
 * its own registers survive the trace.
 *
 * At a depth of two the red surface sees the green one, which sees the red one, which
 * traces no further: red, plus green, plus red. If the levels shared a machine, the outer
 * red would read the inner Cs after its trace returned, and the sum would differ.
 **/
BOOST_AUTO_TEST_CASE(trace_a_trace_recurses_test) {
    const glm::vec3 colour = relayed(2);
    BOOST_CHECK_CLOSE(colour.r, 2.0F, 0.1F);
    BOOST_CHECK_CLOSE(colour.g, 1.0F, 0.1F);
    BOOST_CHECK_SMALL(colour.b, 0.0001F);
}

/**
 * The scene's depth is where the trace stops, and past it a trace returns the background,
 * which is black here.
 **/
BOOST_AUTO_TEST_CASE(trace_a_trace_stops_at_the_depth_test) {
    const glm::vec3 once = relayed(1);
    BOOST_CHECK_CLOSE(once.r, 1.0F, 0.1F);
    BOOST_CHECK_CLOSE(once.g, 1.0F, 0.1F);

    const glm::vec3 never = relayed(0);
    BOOST_CHECK_CLOSE(never.r, 1.0F, 0.1F);
    BOOST_CHECK_SMALL(never.g, 0.0001F);

    const glm::vec3 deeper = relayed(3);
    BOOST_CHECK_CLOSE(deeper.r, 2.0F, 0.1F);
    BOOST_CHECK_CLOSE(deeper.g, 2.0F, 0.1F);
}

namespace {

/** What a ray straight down the negative z axis from a height sees. **/
v3d::render::offline::trace::Tracer::Seen down(v3d::render::offline::trace::Tracer* shader, float x, float y, float from) {
    return shader->see(v3d::type::geometry::Ray({.origin = glm::vec3(x, y, from), .direction = glm::vec3(0.0F, 0.0F, -1.0F)}));
}

/** A triangle in the plane z = height, facing down the negative z axis. **/
v3d::render::offline::trace::Triangle downward(float size, float height, const glm::vec3 & colour) {
    return v3d::render::offline::trace::Triangle({glm::vec3(-size, -size, height), glm::vec3(0.0F, size, height), glm::vec3(size, -size, height)}, colour);
}

};  // namespace

/**
 * A mirror facing a red quad shows the red quad, exactly: shinymetal with its ambient and its
 * highlight off is Cs times what it traces, and the quad has no shader of its own.
 **/
BOOST_AUTO_TEST_CASE(trace_a_mirror_shows_what_it_faces_test) {
    v3d::render::offline::trace::Scene scene;
    v3d::render::offline::trace::Triangle mirror = facing(2.0F, glm::vec3(1.0F));
    ParameterList only;
    add(&only, "Ka", Declaration::Type::FLOAT, { 0.0F });
    add(&only, "Ks", Declaration::Type::FLOAT, { 0.0F });
    mirror.surface(instance("shinymetal", v3d::render::offline::sl::ShaderType::SURFACE, only));
    scene.add(mirror);
    scene.add(downward(4.0F, 2.0F, glm::vec3(1.0F, 0.0F, 0.0F)));

    v3d::render::offline::trace::Tracer shader(&scene);
    const glm::vec3 seen = down(&shader, 0.0F, 0.0F, 1.0F).colour;
    BOOST_CHECK_CLOSE(seen.r, 1.0F, 0.001F);
    BOOST_CHECK_SMALL(seen.g, 1.0e-6F);
    BOOST_CHECK_SMALL(seen.b, 1.0e-6F);
}

/**
 * A half opaque white quad over a black one composites to grey through its Oi, and over
 * nothing it lets half the background through.
 **/
BOOST_AUTO_TEST_CASE(trace_transparency_composites_test) {
    const Placed constant = instance("constant", v3d::render::offline::sl::ShaderType::SURFACE, ParameterList());
    v3d::render::offline::trace::Scene scene;
    scene.background(glm::vec3(0.0F, 0.0F, 1.0F));
    v3d::render::offline::trace::Triangle pane({glm::vec3(-4.0F, -4.0F, 1.0F), glm::vec3(4.0F, -4.0F, 1.0F), glm::vec3(0.0F, 4.0F, 1.0F)}, glm::vec3(1.0F));
    pane.surface(constant);
    pane.opacity(glm::vec3(0.5F));
    scene.add(pane);
    // the black quad is under the left of the pane only
    v3d::render::offline::trace::Triangle under({glm::vec3(-3.0F, -3.0F, 0.0F), glm::vec3(-1.0F, -3.0F, 0.0F), glm::vec3(-1.0F, 3.0F, 0.0F)}, glm::vec3(0.0F));
    under.surface(constant);
    scene.add(under);

    v3d::render::offline::trace::Tracer shader(&scene);
    const v3d::render::offline::trace::Tracer::Seen over = down(&shader, -1.5F, -1.0F, 10.0F);
    BOOST_CHECK(over.hit);
    BOOST_CHECK_CLOSE(over.colour.r, 0.5F, 0.001F);
    BOOST_CHECK_CLOSE(over.colour.b, 0.5F, 0.001F);
    BOOST_CHECK_CLOSE(over.opacity.r, 1.0F, 0.001F);
    BOOST_CHECK_CLOSE(over.distance, 9.0F, 0.001F);

    const v3d::render::offline::trace::Tracer::Seen alone = down(&shader, 1.0F, -1.0F, 10.0F);
    BOOST_CHECK_CLOSE(alone.colour.r, 0.5F, 0.001F);
    BOOST_CHECK_CLOSE(alone.colour.b, 1.0F, 0.001F);
    BOOST_CHECK_CLOSE(alone.opacity.r, 0.5F, 0.001F);
}

/**
 * A shadow through a half opaque occluder is half as dark: the occluder test's scene with
 * the occluder's Os at a half, where its shadow lands.
 **/
BOOST_AUTO_TEST_CASE(trace_a_shadow_through_a_pane_test) {
    v3d::render::offline::trace::Scene scene;
    v3d::render::offline::trace::Triangle floor = facing(8.0F, glm::vec3(1.0F));
    floor.surface(instance("matte", v3d::render::offline::sl::ShaderType::SURFACE, ParameterList()));
    scene.add(floor);
    v3d::render::offline::trace::Triangle pane({glm::vec3(-0.5F, -0.5F, 1.0F), glm::vec3(0.5F, -0.5F, 1.0F), glm::vec3(0.0F, 0.5F, 1.0F)}, glm::vec3(1.0F));
    pane.opacity(glm::vec3(0.5F));
    scene.add(pane);

    ParameterList tilted;
    add(&tilted, "to", Declaration::Type::POINT, { 0.70710678F, 0.0F, -0.70710678F });
    scene.add(instance("distantlight", v3d::render::offline::sl::ShaderType::LIGHT, tilted));

    v3d::render::offline::trace::Tracer shader(&scene);
    BOOST_CHECK_CLOSE(shader.shade(at(scene, 1.0F, 0.0F)).r, 0.5F * 0.70710678F, 0.5F);
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
    v3d::render::offline::trace::Triangle top({glm::vec3(-4.0F, -4.0F, 1.0F), glm::vec3(4.0F, -4.0F, 1.0F), glm::vec3(0.0F, 4.0F, 1.0F)}, glm::vec3(1.0F));
    top.surface(glass);
    scene.add(top);
    v3d::render::offline::trace::Triangle bottom = downward(4.0F, 0.5F, glm::vec3(1.0F));
    bottom.surface(glass);
    scene.add(bottom);
    scene.add(v3d::render::offline::trace::Triangle({glm::vec3(-0.5F, -0.5F, 0.0F), glm::vec3(0.5F, -0.5F, 0.0F), glm::vec3(0.0F, 0.5F, 0.0F)}, glm::vec3(1.0F, 0.0F, 0.0F)));

    v3d::render::offline::trace::Tracer shader(&scene);
    const glm::vec3 through = down(&shader, 0.0F, 0.0F, 10.0F).colour;
    BOOST_CHECK_CLOSE(through.r, 0.96F * 0.96F, 0.01F);
    BOOST_CHECK_SMALL(through.g, 1.0e-6F);
    BOOST_CHECK_SMALL(down(&shader, 0.0F, -0.7F, 10.0F).colour.r, 1.0e-6F);
}

/**
 * A primitive given lights of its own is shaded by those rather than by the scene's. The
 * scene's list is for a primitive that was given none.
 **/
BOOST_AUTO_TEST_CASE(trace_a_primitive_has_its_own_lights_test) {
    v3d::render::offline::trace::Scene scene;
    v3d::render::offline::trace::Triangle triangle = facing(2.0F, glm::vec3(1.0F));
    triangle.surface(instance("matte", v3d::render::offline::sl::ShaderType::SURFACE,
        ParameterList()));

    // the scene's light is overhead and the primitive's own is sixty degrees off
    ParameterList overhead;
    add(&overhead, "to", Declaration::Type::POINT, { 0.0F, 0.0F, -1.0F });
    scene.add(instance("distantlight", v3d::render::offline::sl::ShaderType::LIGHT, overhead));
    ParameterList tilted;
    add(&tilted, "to", Declaration::Type::POINT, { 0.8660254F, 0.0F, -0.5F });
    triangle.lights(boost::make_shared<std::vector<Placed> >(
        1, instance("distantlight", v3d::render::offline::sl::ShaderType::LIGHT, tilted)));
    scene.add(triangle);

    v3d::render::offline::trace::Tracer shader(&scene);
    BOOST_CHECK_CLOSE(shader.shade(at(scene, 0.0F, 0.0F)).r, 0.5F, 0.5F);
}

/**
 * A shader that fails to run leaves the primitive drawn in its own colour, as one with no
 * shader is, and its opacity still applies to that colour.
 **/
BOOST_AUTO_TEST_CASE(trace_a_failed_shader_keeps_its_opacity_test) {
    static ShaderLibrary shaders(boost::make_shared<v3d::log::Logger>());
    shaders.searchpath("data");

    v3d::render::offline::trace::Scene scene;
    v3d::render::offline::trace::Triangle pane = facing(2.0F, glm::vec3(1.0F));
    Placed endless;
    endless.shader = shaders.instance("endless", v3d::render::offline::sl::ShaderType::SURFACE, ParameterList());
    BOOST_REQUIRE(endless.shader);
    pane.surface(endless);
    pane.opacity(glm::vec3(0.5F));
    scene.add(pane);

    v3d::render::offline::trace::Tracer shader(&scene);
    const glm::vec3 colour = shader.shade(at(scene, 0.0F, 0.0F));
    BOOST_CHECK_CLOSE(colour.r, 0.5F, 0.1F);
}

/**
 * A hit has no "object" space, because a traced primitive does not carry the transformation
 * that was in force where the scene created it. The hit refuses the space rather than
 * answering with world space, and the machine reports the refusal. "world" and "current" are
 * the identity.
 **/
BOOST_AUTO_TEST_CASE(trace_a_hit_refuses_object_space_test) {
    v3d::render::offline::trace::Scene scene;
    scene.add(facing(2.0F, glm::vec3(1.0F)));
    v3d::render::offline::trace::Tracer tracer(&scene);
    const v3d::render::offline::trace::Hit hit = at(scene, 0.0F, 0.0F);
    v3d::render::offline::trace::HitShader shader(&tracer, hit);

    glm::mat4x4 matrix(1.0F);
    BOOST_CHECK(!shader.space("object", &matrix));
    BOOST_CHECK(shader.space("world", &matrix));
    BOOST_CHECK((matrix == glm::mat4x4(1.0F)));
    BOOST_CHECK(shader.space("current", &matrix));
    BOOST_CHECK((matrix == glm::mat4x4(1.0F)));
}

/**
 * A shadow ray that ends on a surface is not blocked by that surface. An area light's samples
 * lie on its own geometry, which would otherwise shadow all of its light. The ray leaves a
 * surface facing the triangle, at a slant, so it starts a little nearer the triangle than the
 * point it was cast from.
 **/
BOOST_AUTO_TEST_CASE(trace_a_shadow_ray_ending_on_a_surface_reaches_it_test) {
    v3d::render::offline::trace::Scene scene;
    scene.add(facing(2.0F, glm::vec3(1.0F)));
    v3d::render::offline::trace::Tracer tracer(&scene);

    const glm::vec3 start(1.5F, 0.4F, 3.0F);
    const glm::vec3 end(-0.3F, -0.2F, 0.0F);
    const glm::vec3 passed = tracer.transmitted({.origin = start, .direction = end - start}, glm::vec3(0.0F, 0.0F, -1.0F));
    BOOST_CHECK_EQUAL(passed.r, 1.0F);
    // a ray that goes on past the surface is blocked by it
    const glm::vec3 beyond = tracer.transmitted({.origin = start, .direction = 1.5F * (end - start)},
        glm::vec3(0.0F, 0.0F, -1.0F));
    BOOST_CHECK_EQUAL(beyond.r, 0.0F);
}

};  // namespace
