/**
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 **/

#include <api/render/offline/sl/Compiler.h>
#include <api/render/offline/sl/Emitter.h>
#include <api/render/offline/sl/Parser.h>
#include <api/render/offline/sl/runtime/Machine.h>
#include <api/render/offline/sl/runtime/Program.h>
#include <api/render/offline/sl/runtime/Renderer.h>
#include <api/render/offline/sl/syntax/Shader.h>
#include <api/render/offline/trace/Hit.h>
#include <api/render/offline/trace/HitShader.h>
#include <api/render/offline/trace/Tracer.h>
#include <api/type/geometry/Ray.h>
#include <moya/libmoya/Polygon.h>
#include <moya/libmoya/RayHider.h>
#include <moya/libmoya/RenderContext.h>
#include <moya/libmoya/Vertex.h>
#include <moya/libmoya/reyes/GridShader.h>

#include <sstream>
#include <string>
#include <vector>

#include <boost/make_shared.hpp>
#include <boost/test/unit_test.hpp>

#include <glm/geometric.hpp>
#include <glm/mat4x4.hpp>
#include <glm/vec3.hpp>
#include <glm/vec4.hpp>

namespace {

/** A point off the axis, in camera space, in front of the camera. BODY names the same one. **/
glm::vec3 offAxis() {
    return {0.4F, -0.2F, 2.0F};
}

/**
 * The shader each case runs. It states the point in camera space with a cast, so it is the
 * same point whatever the hider's current space is. It reads the point back in camera space
 * through ptransform, for the same reason.
 **/
const char* const BODY =
"point here = point \"camera\" (0.4, -0.2, 2);\n"
"point raster = ptransform(\"raster\", here);\n"
"float away = depth(here);\n"
"point back = ptransform(\"raster\", \"camera\", ptransform(\"camera\", \"raster\", point (0.4, -0.2, 2)));\n"
"point stated = point \"raster\" (xcomp(raster), ycomp(raster), zcomp(raster));\n"
"point cast = ptransform(\"camera\", stated);\n";

/**
 * What the shader leaves in its variables.
 **/
struct Answers {
    glm::vec3 raster{0.0F};
    float depth = 0.0F;
    glm::vec3 back{0.0F};
    glm::vec3 cast{0.0F};
};

/**
 * BODY run over a batch of one against a renderer, which answers its spaces.
 **/
Answers run(v3d::render::offline::sl::runtime::Renderer* renderer) {
    const std::string source = std::string("surface test() {\n") + BODY + "}\n";
    std::istringstream stream(source);
    v3d::render::offline::sl::Parser parser(stream);
    std::vector<v3d::render::offline::sl::syntax::ShaderPtr> shaders = parser.parse();
    BOOST_REQUIRE_MESSAGE(shaders.size() == 1, parser.error());
    v3d::render::offline::sl::Compiler compiler(shaders[0]);
    BOOST_REQUIRE_MESSAGE(compiler.compile(), compiler.error());
    v3d::render::offline::sl::runtime::Program program;
    v3d::render::offline::sl::Emitter emitter(shaders[0], compiler.symbols());
    BOOST_REQUIRE_MESSAGE(emitter.emit(&program), emitter.error());
    v3d::render::offline::sl::runtime::Machine machine;
    machine.renderer(renderer);
    machine.prepare(program, 1);
    BOOST_REQUIRE_MESSAGE(machine.run(), machine.error());
    BOOST_CHECK(machine.reports().empty());

    Answers answers;
    answers.raster = machine.value(program.symbol("raster")).triple(0);
    answers.depth = machine.value(program.symbol("away")).number(0);
    answers.back = machine.value(program.symbol("back")).triple(0);
    answers.cast = machine.value(program.symbol("cast")).triple(0);
    return answers;
}

/**
 * A 64 by 48 perspective camera with a field of view of 90 degrees, placed off the world
 * origin so that world and camera space differ. The ray hider's scene holds a wall four units
 * in front of the camera, for a hit to shade at.
 **/
void frame(v3d::moya::RenderContext* rc) {
    rc->hider("raytrace");
    rc->imageResolution({64, 48}, 1.0F);
    rc->clipping({.hither = 0.1F, .yon = 100.0F});
    rc->projection("perspective", 90.0F);
    // world to camera, so the camera sits at (-0.5, 0, -2) in world space
    rc->translate(0.5F, 0.0F, 2.0F);
    rc->prepareWorld();

    const boost::shared_ptr<v3d::moya::Polygon> wall = boost::make_shared<v3d::moya::Polygon>();
    for (const glm::vec3 & corner : { glm::vec3(-5.0F, -5.0F, 2.0F), glm::vec3(5.0F, -5.0F, 2.0F),
        glm::vec3(5.0F, 5.0F, 2.0F), glm::vec3(-5.0F, 5.0F, 2.0F) }) {
        v3d::moya::Vertex vertex;
        vertex.point(corner);
        wall->addVertex(vertex);
    }
    rc->addPolygon(wall);
}

/**
 * The hit the camera's central ray makes on the wall.
 **/
v3d::render::offline::trace::Hit centre(v3d::moya::RenderContext* rc) {
    const v3d::type::geometry::Ray ray({.origin = glm::vec3(-0.5F, 0.0F, -2.0F), .direction = glm::vec3(0.0F, 0.0F, 1.0F)});
    v3d::render::offline::trace::Hit hit;
    BOOST_REQUIRE_MESSAGE(rc->traced().nearest(ray, 0.0F, &hit), "the ray missed the wall");
    return hit;
}

/**
 * A point in raster space under a perspective camera is divided by w, and lands on the pixel
 * whose ray passes through it.
 *
 * The screen window is [-4/3, 4/3] by [-1, 1]. At a field of view of 90 degrees the point
 * reaches screen x = 0.4 / 2 and y = -0.2 / 2. Raster x is (0.2 + 4/3) / (8/3) * 64 = 36.8,
 * and raster y is (1 + 0.1) / 2 * 48 = 26.4, counted down from the top.
 **/
BOOST_AUTO_TEST_CASE(moya_raster_space_divides_by_w_test) {
    v3d::moya::RenderContext rc;
    frame(&rc);
    const Answers answers = run(&rc.shader());

    BOOST_CHECK_CLOSE(answers.raster.x, 36.8F, 0.01F);
    BOOST_CHECK_CLOSE(answers.raster.y, 26.4F, 0.01F);

    // the ray the ray hider casts through that raster position passes through the point
    v3d::moya::RayHider::Camera camera;
    camera.toRaster = rc.coordinateSystem("raster") * rc.coordinateSystem("screen");
    camera.perspective = true;
    camera.hither = 0.1F;
    camera.width = 64;
    camera.height = 48;
    v3d::moya::RayHider hider;
    hider.camera(camera);
    const v3d::type::geometry::Ray ray = hider.ray(
        {.raster = glm::vec2(answers.raster), .lens = glm::vec2(0.0F)}, rc.sampling());
    BOOST_CHECK_SMALL(glm::length(glm::cross(glm::normalize(ray.direction()), glm::normalize(offAxis()))), 1.0e-5F);
}

/**
 * depth() under a perspective camera is the NDC z of the divided point: 0 at the near plane
 * and 1 at the far one. The screen matrix's z and w are divided here by hand.
 **/
BOOST_AUTO_TEST_CASE(moya_depth_divides_by_w_test) {
    v3d::moya::RenderContext rc;
    frame(&rc);
    const Answers answers = run(&rc.shader());

    const glm::vec4 screen = rc.coordinateSystem("screen") * glm::vec4(offAxis(), 1.0F);
    const float expected = 0.5F * ((screen.z / screen.w) + 1.0F);
    BOOST_CHECK_CLOSE(answers.depth, expected, 0.01F);
    BOOST_CHECK_GT(answers.depth, 0.0F);
    BOOST_CHECK_LT(answers.depth, 1.0F);
}

/**
 * A point taken into raster space and back is where it started. Back out of a projected space,
 * the inverse projection puts a w other than one into the point, so this also needs the divide.
 * A cast from raster space is the same inverse.
 **/
BOOST_AUTO_TEST_CASE(moya_raster_space_round_trips_test) {
    v3d::moya::RenderContext rc;
    frame(&rc);
    const Answers answers = run(&rc.shader());

    BOOST_CHECK_SMALL(glm::length(answers.back - offAxis()), 1.0e-4F);
    BOOST_CHECK_SMALL(glm::length(answers.cast - offAxis()), 1.0e-4F);
}

/**
 * The ray hider answers the projected spaces from the same camera, so a shader reads the same
 * raster position, depth and round trip under either hider. The hit's current space is world
 * space, which this camera moves off camera space.
 **/
BOOST_AUTO_TEST_CASE(moya_projected_spaces_agree_between_the_hiders_test) {
    v3d::moya::RenderContext rc;
    frame(&rc);
    const Answers grid = run(&rc.shader());

    v3d::render::offline::trace::Tracer tracer(&rc.traced(), &rc.textures());
    v3d::render::offline::trace::HitShader hit(&tracer, centre(&rc));
    const Answers traced = run(&hit);

    BOOST_CHECK_SMALL(glm::length(traced.raster - grid.raster), 1.0e-3F);
    BOOST_CHECK_CLOSE(traced.depth, grid.depth, 0.01F);
    BOOST_CHECK_SMALL(glm::length(traced.back - offAxis()), 1.0e-4F);
    BOOST_CHECK_SMALL(glm::length(traced.cast - offAxis()), 1.0e-4F);
}

};  // namespace
