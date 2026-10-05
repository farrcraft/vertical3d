/**
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 **/

#include <api/render/offline/rib/Reader.h>
#include <api/render/offline/trace/Hit.h>
#include <moya/libmoya/RIBHandler.h>
#include <moya/libmoya/RayHider.h>
#include <moya/libmoya/RenderContext.h>
#include <moya/libmoya/RenderMan.h>

#include <sstream>
#include <string>
#include <vector>

#include <boost/test/unit_test.hpp>
#include <boost/make_shared.hpp>

#include <glm/geometric.hpp>
#include <glm/gtc/matrix_transform.hpp>

namespace {

typedef v3d::moya::FrameBuffer Planes;

/**
 * A 16 by 16 frame of an orthographic camera looking down +z from z = -1. The frame is square,
 * so the screen window covers world x and y over [-1, 1], and a pixel centre is at
 * x = -1 + (column + 0.5) / 8 and y = 1 - (row + 0.5) / 8.
 **/
const unsigned int SIZE = 16;

void frame(v3d::moya::RenderContext & rc) {
    rc.hider("raytrace");
    rc.imageResolution(SIZE, SIZE, 1.0f);
    rc.clipping(0.001f, 100.0f);
    rc.projection("orthographic");
    rc.setTransform(glm::translate(glm::mat4x4(1.0f), glm::vec3(0.0f, 0.0f, 1.0f)));
    // a sample at each pixel centre, given back exactly, so a pixel is what its centre hit
    rc.sampling().samples = glm::uvec2(1, 1);
    rc.sampling().filter = v3d::render::offline::Filter::Box;
    rc.sampling().width = glm::vec2(1.0f, 1.0f);
}

void triangle(v3d::moya::RenderContext & rc, const glm::vec3 & a, const glm::vec3 & b, const glm::vec3 & c,
    const glm::vec3 & colour) {
    rc.color(colour);
    boost::shared_ptr<v3d::moya::Polygon> polygon = boost::make_shared<v3d::moya::Polygon>();
    for (const glm::vec3 & corner : { a, b, c }) {
        v3d::moya::Vertex vertex;
        vertex.point(corner);
        polygon->addVertex(vertex);
    }
    rc.addPolygon(polygon);
}

/**
 * A right triangle with its vertical edge at x = 0.5 and its horizontal one at y = -0.5, so a
 * pixel either side of each edge is half a pixel away from it.
 **/
void rightTriangle(v3d::moya::RenderContext & rc, float edge = 0.5f) {
    triangle(rc, glm::vec3(-1.5f, -0.5f, 1.0f), glm::vec3(edge, -0.5f, 1.0f), glm::vec3(edge, 1.5f, 1.0f),
        glm::vec3(1.0f, 0.0f, 0.0f));
}

void checkPixel(const v3d::render::offline::FrameBuffer & planes,
    unsigned int column, unsigned int row, const glm::vec3 & expected) {
    BOOST_TEST_CONTEXT("pixel " << column << ", " << row) {
        BOOST_CHECK_EQUAL(planes.value(Planes::RED, column, row), expected.r);
        BOOST_CHECK_EQUAL(planes.value(Planes::GREEN, column, row), expected.g);
        BOOST_CHECK_EQUAL(planes.value(Planes::BLUE, column, row), expected.b);
    }
}

bool read(const std::string & source, v3d::moya::RIBHandler * handler) {
    v3d::render::offline::rib::Reader reader(boost::make_shared<v3d::log::Logger>());
    std::istringstream stream(source);
    return reader.read(stream, handler);
}

/**
 * The frame at the RI defaults with a PixelVariance, and a triangle whose vertical edge falls
 * through the middle of column 12 rather than between two columns.
 **/
void adaptive(v3d::moya::RenderContext & rc) {
    frame(rc);
    rc.sampling() = v3d::render::offline::Sampling();
    rc.sampling().variance = 0.001f;
    rc.prepareWorld();
    rc.surface("constant", v3d::render::offline::rib::ParameterList());
    rightTriangle(rc, 0.5625f);
}

};  // namespace

/**
 * RI's default hider is the reyes one, named "hidden". A name moya does not know leaves the
 * hider as it was, so a scene written for another renderer still renders.
 **/
BOOST_AUTO_TEST_CASE(rayhider_hider_is_chosen_by_name_test) {
    v3d::moya::RenderContext rc;
    BOOST_CHECK(!rc.raytracing());
    rc.hider("raytrace");
    BOOST_CHECK(rc.raytracing());
    rc.hider("paint");
    BOOST_CHECK(rc.raytracing());
    rc.hider("hidden");
    BOOST_CHECK(!rc.raytracing());
}

/**
 * RiHider reaches the same context the RIB request does.
 **/
BOOST_AUTO_TEST_CASE(rayhider_renderman_hider_test) {
    RiBegin(RI_NULL);
    RiHider(const_cast<char*>("raytrace"), RI_NULL);
    BOOST_CHECK(static_cast<v3d::moya::RenderContext*>(RiGetContext())->raytracing());
    RiEnd();
}

/**
 * Every ray misses, so every pixel records that nothing was drawn into it. An imager can then
 * tell a pixel the scene never reached from a black one.
 **/
BOOST_AUTO_TEST_CASE(rayhider_empty_scene_test) {
    v3d::moya::RenderContext rc;
    frame(rc);
    rc.prepareWorld();
    rc.render();

    const v3d::render::offline::FrameBuffer & planes = *rc.framebuffer()->planes();
    checkPixel(planes, 8, 8, glm::vec3(0.0f));
    BOOST_CHECK_EQUAL(planes.value(Planes::COVERAGE, 8, 8), 0.0f);
}

BOOST_AUTO_TEST_CASE(rayhider_triangle_test) {
    v3d::moya::RenderContext rc;
    frame(rc);
    rc.prepareWorld();
    rc.surface("constant", v3d::render::offline::rib::ParameterList());
    rightTriangle(rc);
    rc.render();

    const v3d::render::offline::FrameBuffer & planes = *rc.framebuffer()->planes();
    const glm::vec3 red(1.0f, 0.0f, 0.0f);
    const glm::vec3 nothing(0.0f);
    // inside, near the middle of the frame
    checkPixel(planes, 8, 8, red);
    // outside, past the hypotenuse in the upper left corner
    checkPixel(planes, 0, 0, nothing);
    // either side of the vertical edge at x = 0.5, which falls between columns 11 and 12
    checkPixel(planes, 11, 8, red);
    checkPixel(planes, 12, 8, nothing);
    // either side of the horizontal edge at y = -0.5, which falls between rows 11 and 12.
    // Raster space counts y downward from the top, so the larger row is the lower half
    checkPixel(planes, 8, 11, red);
    checkPixel(planes, 8, 12, nothing);
}

/**
 * The ray hider sees the traced scene and nothing else, so a polygon it is given is never
 * bucketed.
 **/
BOOST_AUTO_TEST_CASE(rayhider_buckets_nothing_test) {
    v3d::moya::RenderContext rc;
    frame(rc);
    rc.prepareWorld();
    rightTriangle(rc);

    BOOST_CHECK_EQUAL(rc.framebuffer()->primitiveCount(), 0u);
    BOOST_CHECK_EQUAL(rc.traced().all<v3d::render::offline::trace::Triangle>().size(), 1u);
}

BOOST_AUTO_TEST_CASE(rayhider_nearest_hit_test) {
    v3d::moya::RenderContext rc;
    frame(rc);
    rc.prepareWorld();
    rc.surface("constant", v3d::render::offline::rib::ParameterList());

    const glm::vec3 closer(1.0f, 0.0f, 0.0f);
    const glm::vec3 further(0.0f, 1.0f, 0.0f);
    // two triangles over the same pixel, the first one further from the camera
    triangle(rc, glm::vec3(-1.0f, -1.0f, 5.0f), glm::vec3(1.0f, -1.0f, 5.0f), glm::vec3(0.0f, 1.0f, 5.0f), further);
    triangle(rc, glm::vec3(-1.0f, -1.0f, 2.0f), glm::vec3(1.0f, -1.0f, 2.0f), glm::vec3(0.0f, 1.0f, 2.0f), closer);
    rc.render();

    // the nearer one wins whichever order they were given in
    checkPixel(*rc.framebuffer()->planes(), 8, 8, closer);
}

/**
 * The depth plane holds raster space z under either hider, so a picture's depth means the same
 * thing whichever drew it.
 **/
BOOST_AUTO_TEST_CASE(rayhider_depth_matches_reyes_test) {
    float depth[2] = { 0.0f, 0.0f };
    const char* hiders[2] = { "hidden", "raytrace" };
    for (unsigned int i = 0; i < 2; i++) {
        v3d::moya::RenderContext rc;
        frame(rc);
        rc.hider(hiders[i]);
        rc.prepareWorld();
        rc.surface("constant", v3d::render::offline::rib::ParameterList());
        triangle(rc, glm::vec3(-1.0f, -1.0f, 5.0f), glm::vec3(1.0f, -1.0f, 5.0f), glm::vec3(0.0f, 1.0f, 5.0f),
            glm::vec3(1.0f));
        rc.render();
        depth[i] = rc.framebuffer()->planes()->value(Planes::DEPTH, 8, 8);
    }
    BOOST_CHECK_CLOSE(depth[1], depth[0], 0.01f);
}

/**
 * At the RI defaults a pixel beside an edge is partly covered: the gaussian is two pixels
 * wide, so the samples of the pixels either side reach it. One well inside is covered whole.
 **/
BOOST_AUTO_TEST_CASE(rayhider_sampled_edge_coverage_test) {
    v3d::moya::RenderContext rc;
    frame(rc);
    rc.sampling() = v3d::render::offline::Sampling();
    rc.prepareWorld();
    rightTriangle(rc);
    rc.render();

    const v3d::render::offline::FrameBuffer & planes = *rc.framebuffer()->planes();
    const float inside = planes.value(Planes::COVERAGE, 11, 8);
    const float outside = planes.value(Planes::COVERAGE, 12, 8);
    BOOST_CHECK(inside > 0.5f && inside < 1.0f);
    BOOST_CHECK(outside > 0.0f && outside < 0.5f);
    BOOST_CHECK_CLOSE(planes.value(Planes::COVERAGE, 8, 8), 1.0f, 1.0e-4f);
    BOOST_CHECK_EQUAL(planes.value(Planes::COVERAGE, 0, 15), 0.0f);
}

/**
 * A pixel whose samples agree takes the first set and no more, and one on an edge, whose
 * samples are red and black, takes more - up to four times the first set.
 **/
BOOST_AUTO_TEST_CASE(rayhider_adaptive_sampling_test) {
    v3d::moya::RenderContext rc;
    adaptive(rc);
    rc.render();

    BOOST_CHECK_EQUAL(rc.samplesTaken(5, 8), 4u);
    BOOST_CHECK_EQUAL(rc.samplesTaken(14, 8), 4u);
    BOOST_CHECK_GT(rc.samplesTaken(12, 8), 4u);
    BOOST_CHECK_LE(rc.samplesTaken(12, 8), 16u);

    // and a variance of zero, the default, takes the first set everywhere
    v3d::moya::RenderContext plain;
    adaptive(plain);
    plain.sampling().variance = 0.0f;
    plain.render();
    BOOST_CHECK_EQUAL(plain.samplesTaken(12, 8), 4u);
}

/**
 * Every further set is seeded by its pixel and its pass, so an adapted render is the same
 * twice.
 **/
BOOST_AUTO_TEST_CASE(rayhider_adaptive_is_repeatable_test) {
    v3d::moya::RenderContext first;
    adaptive(first);
    first.render();
    v3d::moya::RenderContext second;
    adaptive(second);
    second.render();

    const v3d::render::offline::FrameBuffer & a = *first.framebuffer()->planes();
    const v3d::render::offline::FrameBuffer & b = *second.framebuffer()->planes();
    for (unsigned int plane = 0; plane < a.planes(); plane++) {
        for (unsigned int row = 0; row < SIZE; row++) {
            for (unsigned int column = 0; column < SIZE; column++) {
                BOOST_REQUIRE_EQUAL(a.value(plane, column, row), b.value(plane, column, row));
            }
        }
    }
}

/**
 * An off centre screen window is a camera like any other: here it covers world x over [0, 2],
 * so a quad on the left of the world is out of the picture and one at x in [0, 1] fills the
 * left half of it.
 **/
BOOST_AUTO_TEST_CASE(rayhider_off_centre_screen_window_test) {
    v3d::moya::Renderer renderer;
    v3d::moya::RIBHandler handler(&renderer);
    BOOST_REQUIRE(read(
        "Hider \"raytrace\"\n"
        "Format 16 16 1\n"
        "PixelSamples 1 1\n"
        "PixelFilter \"box\" 1 1\n"
        "ScreenWindow 0 2 -1 1\n"
        "Projection \"orthographic\"\n"
        "Clipping 0.001 100\n"
        "WorldBegin\n"
        "Polygon \"P\" [0 -1 2  1 -1 2  1 1 2  0 1 2]\n"
        "WorldEnd\n", &handler));

    const v3d::render::offline::FrameBuffer & planes = *handler.context().framebuffer()->planes();
    BOOST_CHECK_EQUAL(planes.value(Planes::COVERAGE, 3, 8), 1.0f);
    BOOST_CHECK_EQUAL(planes.value(Planes::COVERAGE, 12, 8), 0.0f);
}

/**
 * A world to camera matrix that is not a rotation and a translation is a camera like any
 * other: one that doubles the world puts a quad half a unit across over a whole unit of
 * screen.
 **/
BOOST_AUTO_TEST_CASE(rayhider_scaling_camera_test) {
    v3d::moya::Renderer renderer;
    v3d::moya::RIBHandler handler(&renderer);
    BOOST_REQUIRE(read(
        "Hider \"raytrace\"\n"
        "Format 16 16 1\n"
        "PixelSamples 1 1\n"
        "PixelFilter \"box\" 1 1\n"
        "Projection \"orthographic\"\n"
        "Clipping 0.001 100\n"
        "Transform [2 0 0 0  0 2 0 0  0 0 2 0  0 0 1 1]\n"
        "WorldBegin\n"
        "Polygon \"P\" [0 -0.5 2  0.5 -0.5 2  0.5 0.5 2  0 0.5 2]\n"
        "WorldEnd\n", &handler));

    // screen x = 2 * world x, so the quad covers screen x over [0, 1], which is columns 8 to 15
    const v3d::render::offline::FrameBuffer & planes = *handler.context().framebuffer()->planes();
    BOOST_CHECK_EQUAL(planes.value(Planes::COVERAGE, 9, 8), 1.0f);
    BOOST_CHECK_EQUAL(planes.value(Planes::COVERAGE, 14, 8), 1.0f);
    BOOST_CHECK_EQUAL(planes.value(Planes::COVERAGE, 6, 8), 0.0f);
}

/**
 * A polygon reaches the traced scene fanned into triangles, through the current transformation:
 * the scene holds them in world space, and a transform block puts back what it saved.
 **/
BOOST_AUTO_TEST_CASE(rayhider_polygon_is_fanned_test) {
    v3d::moya::Renderer renderer;
    v3d::moya::RIBHandler handler(&renderer);
    BOOST_REQUIRE(read(
        "Hider \"raytrace\"\n"
        "Format 32 16 1\n"
        "WorldBegin\n"
        "TransformBegin\n"
        "Translate 10 10 10\n"
        "TransformEnd\n"
        "Translate 0 0 1\n"
        "Polygon \"P\" [0 0 1  1 0 1  1 1 1  0 1 1]\n"
        "WorldEnd\n", &handler));

    const std::vector<const v3d::render::offline::trace::Triangle*> triangles =
        handler.context().traced().all<v3d::render::offline::trace::Triangle>();
    BOOST_REQUIRE_EQUAL(triangles.size(), 2u);
    BOOST_CHECK_EQUAL(triangles[0]->a().x, 0.0f);
    BOOST_CHECK_EQUAL(triangles[0]->a().z, 2.0f);
}

/**
 * A polygon that says nothing about its normals takes its own plane, in world space. A varying
 * "N" is the shading normal and overrides it, and Ng stays the plane.
 **/
BOOST_AUTO_TEST_CASE(rayhider_normals_test) {
    v3d::moya::Renderer renderer;
    v3d::moya::RIBHandler handler(&renderer);
    BOOST_REQUIRE(read(
        "Hider \"raytrace\"\n"
        "Format 32 16 1\n"
        "WorldBegin\n"
        "Polygon \"P\" [0 0 0  1 0 0  0 1 0]\n"
        "Polygon \"P\" [0 0 0  1 0 0  0 1 0] \"N\" [0 1 0  0 1 0  0 1 0]\n"
        "WorldEnd\n", &handler));

    const std::vector<const v3d::render::offline::trace::Triangle*> triangles =
        handler.context().traced().all<v3d::render::offline::trace::Triangle>();
    BOOST_REQUIRE_EQUAL(triangles.size(), 2u);
    BOOST_TEST((triangles[0]->geometricNormal() == glm::vec3(0.0f, 0.0f, 1.0f)));
    BOOST_TEST((triangles[0]->shadingNormal(0.25f, 0.25f) == glm::vec3(0.0f, 0.0f, 1.0f)));
    BOOST_TEST((triangles[1]->shadingNormal(0.25f, 0.25f) == glm::vec3(0.0f, 1.0f, 0.0f)));
    BOOST_TEST((triangles[1]->geometricNormal() == glm::vec3(0.0f, 0.0f, 1.0f)));
}

/**
 * A normal transforms by the inverse transpose of the current transformation, not by the matrix
 * that moves the points. Only a scene that scales one axis tells them apart, and there the
 * normal leans the opposite way to the points.
 **/
BOOST_AUTO_TEST_CASE(rayhider_normal_inverse_transpose_test) {
    v3d::moya::Renderer renderer;
    v3d::moya::RIBHandler handler(&renderer);
    BOOST_REQUIRE(read(
        "Hider \"raytrace\"\n"
        "Format 32 16 1\n"
        "WorldBegin\n"
        "Scale 1 2 1\n"
        "Polygon \"P\" [1 0 0  0 1 0  0 1 1] \"N\" [1 1 0  1 1 0  1 1 0]\n"
        "WorldEnd\n", &handler));

    const std::vector<const v3d::render::offline::trace::Triangle*> triangles =
        handler.context().traced().all<v3d::render::offline::trace::Triangle>();
    BOOST_REQUIRE_EQUAL(triangles.size(), 1u);
    // (1, 1, 0) under the inverse transpose of a scale of two in y has its y halved
    const glm::vec3 expected = glm::normalize(glm::vec3(1.0f, 0.5f, 0.0f));
    const glm::vec3 shading = triangles[0]->shadingNormal(0.25f, 0.25f);
    BOOST_TEST(shading.x == expected.x, boost::test_tools::tolerance(0.0001f));
    BOOST_TEST(shading.y == expected.y, boost::test_tools::tolerance(0.0001f));
    // the "N" given was the polygon's own plane, so the plane the moved points lie in agrees
    BOOST_TEST(triangles[0]->geometricNormal().x == expected.x, boost::test_tools::tolerance(0.0001f));
    BOOST_TEST(triangles[0]->geometricNormal().y == expected.y, boost::test_tools::tolerance(0.0001f));
}

/**
 * A sphere reaches the traced scene with the colour, opacity and transformation that were
 * current, and is met where the transformation put it.
 **/
BOOST_AUTO_TEST_CASE(rayhider_sphere_test) {
    v3d::moya::Renderer renderer;
    v3d::moya::RIBHandler handler(&renderer);
    BOOST_REQUIRE(read(
        "Hider \"raytrace\"\n"
        "Format 32 16 1\n"
        "WorldBegin\n"
        "Color [0.2 0.4 0.6]\n"
        "Opacity [0.5 0.5 0.5]\n"
        "Translate 0 0 5\n"
        "Sphere 1 -1 1 360\n"
        "WorldEnd\n", &handler));

    const v3d::render::offline::trace::Scene & scene = handler.context().traced();
    BOOST_REQUIRE_EQUAL(scene.all<v3d::render::offline::trace::Sphere>().size(), 1u);
    BOOST_CHECK_CLOSE(scene.all<v3d::render::offline::trace::Sphere>()[0]->colour().g, 0.4f, 0.001f);
    BOOST_CHECK_CLOSE(scene.all<v3d::render::offline::trace::Sphere>()[0]->opacity().r, 0.5f, 0.001f);

    v3d::render::offline::trace::Hit hit;
    BOOST_REQUIRE(scene.nearest(v3d::type::geometry::Ray(glm::vec3(0.0f), glm::vec3(0.0f, 0.0f, 1.0f)), 0.0f, &hit));
    BOOST_CHECK_CLOSE(hit.distance, 4.0f, 0.001f);
}

/**
 * A primitive is given the lights that are on when it is made, so switching one off between
 * two primitives lights them differently. Primitives made under the same lights share one set.
 **/
BOOST_AUTO_TEST_CASE(rayhider_lights_per_primitive_test) {
    v3d::moya::Renderer renderer;
    v3d::moya::RIBHandler handler(&renderer);
    BOOST_REQUIRE(read(
        "Hider \"raytrace\"\n"
        "Format 32 16 1\n"
        "WorldBegin\n"
        "LightSource \"ambientlight\" 1\n"
        "LightSource \"distantlight\" 2\n"
        "Polygon \"P\" [0 0 5  1 0 5  0 1 5]\n"
        "Polygon \"P\" [0 0 6  1 0 6  0 1 6]\n"
        "Illuminate 2 0\n"
        "Polygon \"P\" [0 0 7  1 0 7  0 1 7]\n"
        "WorldEnd\n", &handler));

    const std::vector<const v3d::render::offline::trace::Triangle*> triangles =
        handler.context().traced().all<v3d::render::offline::trace::Triangle>();
    BOOST_REQUIRE_EQUAL(triangles.size(), 3u);
    BOOST_REQUIRE(triangles[0]->lights());
    BOOST_CHECK_EQUAL(triangles[0]->lights()->size(), 2u);
    BOOST_CHECK(triangles[1]->lights() == triangles[0]->lights());
    BOOST_REQUIRE(triangles[2]->lights());
    BOOST_CHECK_EQUAL(triangles[2]->lights()->size(), 1u);
}

/**
 * A primary ray is found by inverting the projection the reyes hider projects through, so a
 * point along it projects back to the raster position it was cast through - off the middle
 * of an uncentred screen window too - and it starts on the near plane.
 **/
BOOST_AUTO_TEST_CASE(rayhider_inverts_the_projection_test) {
    v3d::moya::RenderContext rc;
    rc.imageResolution(32, 16, 1.0f);
    rc.screenWindow(-0.5f, 1.5f, -1.0f, 1.0f);
    rc.clipping(0.1f, 100.0f);
    rc.projection("perspective", 60.0f);

    v3d::moya::RayHider::Camera camera;
    camera.toRaster = rc.coordinateSystem("raster") * rc.coordinateSystem("screen");
    camera.perspective = true;
    camera.hither = 0.1f;
    camera.width = 32;
    camera.height = 16;
    v3d::moya::RayHider hider;
    hider.camera(camera);

    for (const glm::vec2 & raster : { glm::vec2(3.5f, 2.5f), glm::vec2(30.0f, 14.0f), glm::vec2(16.0f, 8.0f) }) {
        const v3d::type::geometry::Ray ray = hider.ray(raster, glm::vec2(0.0f), rc.sampling());
        BOOST_CHECK_CLOSE(ray.origin().z, 0.1f, 0.01f);
        const glm::vec3 projected = v3d::moya::project(camera.toRaster, ray.origin() + ray.direction() * 5.0f);
        BOOST_CHECK_CLOSE(projected.x, raster.x, 0.01f);
        BOOST_CHECK_CLOSE(projected.y, raster.y, 0.01f);
    }
}

/**
 * A polygon with a colour at each vertex is shaded with those colours blended across it, by
 * both hiders, and the two agree to within the reyes hider's micropolygon size.
 **/
BOOST_AUTO_TEST_CASE(rayhider_vertex_colours_test) {
    const char* hiders[2] = { "hidden", "raytrace" };
    glm::vec3 seen[2][2];
    for (unsigned int i = 0; i < 2; i++) {
        v3d::moya::RenderContext rc;
        frame(rc);
        rc.hider(hiders[i]);
        rc.prepareWorld();
        rc.surface("constant", v3d::render::offline::rib::ParameterList());
        rc.color(glm::vec3(1.0f));
        boost::shared_ptr<v3d::moya::Polygon> polygon = boost::make_shared<v3d::moya::Polygon>();
        const glm::vec3 corners[3] = { glm::vec3(-1.0f, -1.0f, 1.0f), glm::vec3(1.0f, -1.0f, 1.0f),
            glm::vec3(-1.0f, 1.0f, 1.0f) };
        const glm::vec3 colours[3] = { glm::vec3(1.0f, 0.0f, 0.0f), glm::vec3(0.0f, 1.0f, 0.0f),
            glm::vec3(0.0f, 0.0f, 1.0f) };
        for (unsigned int k = 0; k < 3; k++) {
            v3d::moya::Vertex vertex;
            vertex.point(corners[k]);
            vertex.color(colours[k]);
            polygon->addVertex(vertex);
        }
        rc.addPolygon(polygon);
        rc.render();

        const v3d::render::offline::FrameBuffer & planes = *rc.framebuffer()->planes();
        // near the red corner at the lower left, and near the green one at the lower right
        const unsigned int near[2][2] = { { 1, 14 }, { 12, 14 } };
        for (unsigned int k = 0; k < 2; k++) {
            seen[i][k] = glm::vec3(planes.value(Planes::RED, near[k][0], near[k][1]),
                planes.value(Planes::GREEN, near[k][0], near[k][1]), planes.value(Planes::BLUE, near[k][0], near[k][1]));
        }
    }
    // the ray hider shades each pixel centre exactly. The one near the red corner is at
    // x = y = -0.8125, which is 0.09375 of the way to each of the other two corners
    BOOST_CHECK_CLOSE(seen[1][0].r, 0.8125f, 0.1f);
    BOOST_CHECK_CLOSE(seen[1][0].g, 0.09375f, 0.1f);
    BOOST_CHECK_CLOSE(seen[1][0].b, 0.09375f, 0.1f);
    BOOST_CHECK_CLOSE(seen[1][1].g, 0.78125f, 0.1f);
    // the reyes hider shades the corners of micropolygons and blends across each one, so it
    // is close to the exact colour rather than equal to it
    for (unsigned int k = 0; k < 2; k++) {
        BOOST_CHECK_SMALL(glm::length(seen[1][k] - seen[0][k]), 0.1f);
    }
}

/**
 * A motion that starts from nothing, here a quad whose width grows from zero while the
 * shutter is open, is drawn by both hiders. The quad is centred on the frame, so it covers
 * the centre pixel at every time but the first.
 **/
BOOST_AUTO_TEST_CASE(rayhider_motion_from_nothing_test) {
    const char* hiders[2] = { "hidden", "raytrace" };
    for (const char* hider : hiders) {
        BOOST_TEST_CONTEXT("hider " << hider) {
            v3d::moya::Renderer renderer;
            v3d::moya::RIBHandler handler(&renderer);
            BOOST_REQUIRE(read(
                std::string("Hider \"") + hider + "\"\n"
                "Format 16 16 1\n"
                "PixelSamples 4 4\n"
                "PixelFilter \"box\" 1 1\n"
                "Shutter 0 1\n"
                "Projection \"orthographic\"\n"
                "Clipping 0.001 100\n"
                "WorldBegin\n"
                "MotionBegin [0 1]\n"
                "Scale 0 1 1\n"
                "Scale 1 1 1\n"
                "MotionEnd\n"
                "Polygon \"P\" [-0.5 -0.5 2  0.5 -0.5 2  0.5 0.5 2  -0.5 0.5 2]\n"
                "WorldEnd\n", &handler));

            const v3d::render::offline::FrameBuffer & planes = *handler.context().framebuffer()->planes();
            BOOST_CHECK_GT(planes.value(Planes::COVERAGE, 8, 8), 0.9f);
        }
    }
}
