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
#include <api/render/offline/trace/HitShader.h>
#include <api/render/offline/trace/Primitive.h>
#include <api/render/offline/trace/Scene.h>
#include <api/render/offline/trace/Sphere.h>
#include <api/render/offline/trace/Tracer.h>
#include <api/render/offline/trace/Triangle.h>
#include <api/type/geometry/Line.h>
#include <api/type/geometry/Ray.h>

#include <cstddef>
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
typedef v3d::render::offline::sl::ShaderType ShaderType;
typedef v3d::render::offline::trace::Scene Scene;
typedef v3d::render::offline::trace::Tracer Tracer;

/** A uniform parameter of a given type, for a shader's parameter list. **/
void bind(ParameterList* parameters, const std::string & name, Declaration::Type type, const std::vector<float> & values) {
    parameters->add(name, Declaration(Declaration::Storage::UNIFORM, type, 1), values, std::vector<std::string>());
}

/** A standard shader, or one of the suite's fixtures, with the parameters given bound. **/
v3d::render::offline::sl::Placed shader(const std::string & name, ShaderType type,
    const ParameterList & parameters = ParameterList()) {
    static v3d::render::offline::sl::ShaderLibrary shaders(boost::make_shared<v3d::log::Logger>());
    shaders.searchpath("data:&");
    v3d::render::offline::sl::Placed placed;
    placed.shader = shaders.instance(name, type, parameters);
    BOOST_REQUIRE_MESSAGE(placed.shader, "no shader named " + name);
    return placed;
}

/**
 * What a run of a shader holds uniform that a primitive sets: its object space, the shader's
 * placement and its lights. Empty lights are the scene's.
 **/
struct Held {
    glm::mat4x4 object = glm::mat4x4(1.0F);
    glm::mat4x4 placement = glm::mat4x4(1.0F);
    v3d::render::offline::trace::Lights lights;
};

/** A camera whose raster space is world space scaled up, so a pixel is 1 / scale wide. **/
void camera(Scene* scene, float scale) {
    v3d::render::offline::Camera camera;
    camera.raster = glm::scale(glm::mat4x4(1.0F), glm::vec3(scale));
    scene->camera(camera);
}

/**
 * A scene with something for every path through a batch: a matte floor that more than one
 * batch of rays meets, two tilted triangles that share one differentiating shader, a matte
 * pane half seen through, and a mirrored sphere that traces what it reflects.
 **/
Scene mixed() {
    Scene scene;
    // a square of two triangles with one instance, so that more hits than a run holds share it
    const v3d::render::offline::sl::Placed ground = shader("matte", ShaderType::SURFACE);
    v3d::render::offline::trace::Triangle lower({glm::vec3(-6.0F, -6.0F, 0.0F), glm::vec3(6.0F, -6.0F, 0.0F),
        glm::vec3(6.0F, 6.0F, 0.0F)}, glm::vec3(0.8F, 0.6F, 0.4F));
    lower.surface(ground);
    scene.add(lower);
    v3d::render::offline::trace::Triangle upper({glm::vec3(-6.0F, -6.0F, 0.0F), glm::vec3(6.0F, 6.0F, 0.0F),
        glm::vec3(-6.0F, 6.0F, 0.0F)}, glm::vec3(0.8F, 0.6F, 0.4F));
    upper.surface(ground);
    scene.add(upper);

    // one instance for both, so their hits are shaded in one run, each with its own steps
    ParameterList which;
    bind(&which, "which", Declaration::Type::FLOAT, { 0.0F });
    const v3d::render::offline::sl::Placed slopes = shader("derivatives", ShaderType::SURFACE, which);
    v3d::render::offline::trace::Triangle steep({glm::vec3(-2.8F, -2.0F, 1.0F), glm::vec3(-1.6F, -1.8F, 1.6F),
        glm::vec3(-2.6F, -0.6F, 1.1F)}, glm::vec3(1.0F));
    steep.surface(slopes);
    scene.add(steep);
    v3d::render::offline::trace::Triangle shallow({glm::vec3(-2.8F, 0.4F, 1.0F), glm::vec3(-0.8F, 0.5F, 1.05F),
        glm::vec3(-2.7F, 2.2F, 1.2F)}, glm::vec3(1.0F));
    shallow.surface(slopes);
    scene.add(shallow);

    v3d::render::offline::trace::Triangle pane({glm::vec3(-1.0F, -2.5F, 2.0F), glm::vec3(1.0F, -2.5F, 2.0F),
        glm::vec3(0.0F, 2.5F, 2.0F)}, glm::vec3(0.2F, 0.4F, 0.9F));
    pane.surface(shader("matte", ShaderType::SURFACE));
    pane.opacity(glm::vec3(0.5F));
    scene.add(pane);

    v3d::render::offline::trace::Sphere ball({.radius = 1.0F, .zmin = -1.0F, .zmax = 1.0F, .thetamax = 360.0F},
        glm::translate(glm::mat4x4(1.0F), glm::vec3(2.0F, 0.0F, 1.0F)), glm::vec3(0.9F));
    ParameterList mirror;
    bind(&mirror, "Kr", Declaration::Type::FLOAT, { 0.8F });
    ball.surface(shader("shinymetal", ShaderType::SURFACE, mirror));
    scene.add(ball, v3d::render::offline::MovingTransform());

    // six narrow triangles of one shader instance, one under each of six rays. Each pair
    // differs in one thing a run holds uniform: the object space, the shader's placement, or
    // the lights. A hit that shared a run with the other of its pair would take its value
    const v3d::render::offline::sl::Placed placed = shader("placedpoint", ShaderType::SURFACE);
    const auto narrow = [&scene, &placed](float x, const Held & held) {
        v3d::render::offline::trace::Triangle thin({glm::vec3(x - 0.2F, 1.6F, 0.5F), glm::vec3(x + 0.2F, 1.6F, 0.5F),
            glm::vec3(x, 2.45F, 0.5F)}, glm::vec3(1.0F));
        v3d::render::offline::sl::Placed moved = placed;
        moved.placement = held.placement;
        thin.surface(moved);
        thin.object(held.object);
        thin.lights(held.lights);
        scene.add(thin);
    };
    const glm::mat4x4 shifted = glm::translate(glm::mat4x4(1.0F), glm::vec3(0.5F, -1.0F, 0.0F));
    ParameterList overhead;
    bind(&overhead, "to", Declaration::Type::POINT, { 0.0F, 0.0F, -1.0F });
    bind(&overhead, "intensity", Declaration::Type::FLOAT, { 0.5F });
    const v3d::render::offline::trace::Lights dim = boost::make_shared<const std::vector<v3d::render::offline::sl::Placed>>(
        std::vector<v3d::render::offline::sl::Placed>{ shader("distantlight", ShaderType::LIGHT, overhead) });
    narrow(0.75F, Held());
    narrow(1.25F, {.object = shifted});
    narrow(1.75F, Held());
    narrow(2.25F, {.placement = shifted});
    narrow(2.75F, Held());
    narrow(3.25F, {.lights = dim});

    // down onto the floor and across it, so the pane and the sphere each cast a shadow there
    ParameterList slanted;
    bind(&slanted, "to", Declaration::Type::POINT, { 0.4F, -0.3F, -1.0F });
    scene.add(shader("distantlight", ShaderType::LIGHT, slanted));
    camera(&scene, 40.0F);
    return scene;
}

/** Rays straight down -z over a grid of points, more than one batch holds. **/
std::vector<Tracer::Look> grid() {
    std::vector<Tracer::Look> looks;
    for (int row = 0; row < 12; row++) {
        for (int column = 0; column < 14; column++) {
            const glm::vec3 from(-3.25F + (0.5F * static_cast<float>(column)), -2.75F + (0.5F * static_cast<float>(row)), 10.0F);
            Tracer::Look look;
            look.ray = v3d::type::geometry::Ray({.origin = from, .direction = glm::vec3(0.0F, 0.0F, -1.0F)});
            looks.push_back(look);
        }
    }
    return looks;
}

BOOST_AUTO_TEST_SUITE(trace_batch_test)

/**
 * A ray sees the same through a batch as alone, to the last bit. The batch mixes shaders,
 * shades more hits of one shader than one run holds, gives two hits of one run their own
 * derivatives and their own shadows, composites a surface it sees through before the floor
 * behind it, and traces a mirror's reflections as a batch of their own. It also holds hits of
 * one shader instance that differ in placement, object space and lights. A batch that let one
 * hit's state reach another's, or composited out of order, would change some ray's answer.
 **/
BOOST_AUTO_TEST_CASE(a_batch_sees_what_each_ray_sees_alone) {
    const Scene scene = mixed();
    const std::vector<Tracer::Look> looks = grid();
    BOOST_REQUIRE_GT(looks.size(), 2U * Tracer::BATCH);

    Tracer batched(&scene);
    const std::vector<Tracer::Seen> together = batched.see(looks);
    BOOST_REQUIRE_EQUAL(together.size(), looks.size());

    Tracer alone(&scene);
    unsigned int hits = 0;
    for (std::size_t i = 0; i < looks.size(); i++) {
        const Tracer::Seen seen = alone.see(looks[i].ray);
        BOOST_TEST_CONTEXT("ray " << i) {
            BOOST_TEST(together[i].hit == seen.hit);
            BOOST_TEST(together[i].distance == seen.distance);
            BOOST_TEST(together[i].colour.r == seen.colour.r);
            BOOST_TEST(together[i].colour.g == seen.colour.g);
            BOOST_TEST(together[i].colour.b == seen.colour.b);
            BOOST_TEST(together[i].opacity.r == seen.opacity.r);
        }
        hits += seen.hit ? 1U : 0U;
    }
    // the grid is over the floor, so every ray meets something
    BOOST_TEST(hits == looks.size());
}

/**
 * A hit is shaded as one point unless its shader reads a derivative, or a light shining on it
 * does. Only then are its two neighbours shaded with it.
 **/
BOOST_AUTO_TEST_CASE(a_hit_is_one_point_unless_its_shader_differentiates) {
    const auto lanes = [](const std::string & surface, const std::string & light) {
        Scene scene;
        v3d::render::offline::trace::Triangle triangle({glm::vec3(-2.0F, -2.0F, 0.0F), glm::vec3(2.0F, -2.0F, 0.0F),
            glm::vec3(0.0F, 2.0F, 0.0F)}, glm::vec3(1.0F));
        triangle.surface(shader(surface, ShaderType::SURFACE));
        scene.add(triangle);
        scene.add(shader(light, ShaderType::LIGHT));
        camera(&scene, 40.0F);
        v3d::render::offline::trace::Hit hit;
        BOOST_REQUIRE(scene.nearest(v3d::type::geometry::Ray({.origin = glm::vec3(0.0F, 0.0F, 5.0F),
            .direction = glm::vec3(0.0F, 0.0F, -1.0F)}), 0.0F, &hit));
        Tracer tracer(&scene);
        v3d::render::offline::trace::HitShader shaded(&tracer, hit);
        glm::vec3 opacity(1.0F);
        shaded.shade(&opacity);
        return shaded.lanes();
    };
    BOOST_TEST(lanes("matte", "distantlight") == 1U);
    BOOST_TEST(lanes("derivatives", "distantlight") == Tracer::STENCIL);
    BOOST_TEST(lanes("matte", "stepped") == Tracer::STENCIL);
}

/**
 * traced() for a batch gives each ray what it gives the ray alone, and a ray of no length, or
 * one past the scene's trace depth, the background.
 **/
BOOST_AUTO_TEST_CASE(a_batch_of_traced_rays_is_each_ray_traced_alone) {
    Scene scene = mixed();
    scene.background(glm::vec3(0.1F, 0.2F, 0.3F));
    std::vector<v3d::type::geometry::Line> paths;
    std::vector<glm::vec3> geometric;
    for (const Tracer::Look & look : grid()) {
        paths.push_back({.origin = look.ray.origin(), .direction = look.ray.direction()});
        geometric.emplace_back(0.0F);
    }
    paths.push_back({.origin = glm::vec3(0.0F, 0.0F, 10.0F), .direction = glm::vec3(0.0F)});
    geometric.emplace_back(0.0F);

    Tracer batched(&scene);
    const std::vector<glm::vec3> together = batched.traced(paths, geometric, std::vector<float>(paths.size(), 0.0F));
    BOOST_REQUIRE_EQUAL(together.size(), paths.size());
    Tracer alone(&scene);
    for (std::size_t i = 0; i < paths.size(); i++) {
        const glm::vec3 seen = alone.traced(paths[i], geometric[i]);
        BOOST_TEST_CONTEXT("ray " << i) {
            BOOST_TEST(together[i].r == seen.r);
            BOOST_TEST(together[i].g == seen.g);
            BOOST_TEST(together[i].b == seen.b);
        }
    }
    BOOST_TEST(together.back().b == 0.3F);

    scene.traceDepth(0);
    Tracer shallow(&scene);
    const std::vector<glm::vec3> none = shallow.traced(paths, geometric, std::vector<float>(paths.size(), 0.0F));
    BOOST_TEST(none.front().g == 0.2F);
}

BOOST_AUTO_TEST_SUITE_END()

};  // namespace
