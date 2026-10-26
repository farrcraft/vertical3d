/**
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 **/

#include <api/log/Logger.h>
#include <api/render/offline/FrameBuffer.h>
#include <api/render/offline/trace/Hit.h>
#include <api/render/offline/trace/Scene.h>
#include <api/type/geometry/Ray.h>
#include <api/render/offline/trace/Sphere.h>
#include <moya/libmoya/Planes.h>
#include <moya/libmoya/RIBHandler.h>
#include <moya/libmoya/Renderer.h>
#include <spdlog/sinks/ringbuffer_sink.h>

#include <algorithm>
#include <cstddef>
#include <memory>
#include <sstream>
#include <string>
#include <utility>
#include <vector>

#include <boost/test/unit_test.hpp>
#include <boost/make_shared.hpp>

#include <glm/glm.hpp>

#include "Handling.h"

namespace v3d::moya::handling {
namespace {

/**
 * The polygon a file names reaches RenderContext::addPolygon(), which files it in a bucket.
 **/
BOOST_AUTO_TEST_CASE(moya_ribhandler_polygon_test) {
    v3d::moya::Renderer renderer;
    v3d::moya::RIBHandler handler(&renderer);

    BOOST_REQUIRE(read(
        "Format 64 48 1\n"
        "Projection \"orthographic\"\n"
        "Clipping 1 100\n"
        "WorldBegin\n"
        "Polygon \"P\" [-0.2 -0.2 5  0.2 -0.2 5  0.2 0.2 5  -0.2 0.2 5]\n"
        "WorldEnd\n", &handler));

    BOOST_CHECK_EQUAL(handler.context().bucketed(), 1U);
}

/**
 * The reyes hider dices polygons only. A scene with a sphere in it renders without the sphere
 * and logs a warning. The ray hider draws spheres.
 **/
BOOST_AUTO_TEST_CASE(moya_ribhandler_sphere_is_skipped_test) {
    v3d::moya::Renderer renderer;
    v3d::moya::RIBHandler handler(&renderer);
    const Heard heard(handler.context().logger()->get());

    BOOST_REQUIRE(read(
        "Format 64 48 1\n"
        "Projection \"orthographic\"\n"
        "Clipping 1 100\n"
        "WorldBegin\n"
        "Sphere 1 -1 1 360\n"
        "Polygon \"P\" [-0.2 -0.2 5  0.2 -0.2 5  0.2 0.2 5  -0.2 0.2 5]\n"
        "WorldEnd\n", &handler));

    BOOST_CHECK_EQUAL(handler.context().bucketed(), 1U);
    BOOST_TEST(heard.said("does not dice spheres"));
}

/**
 * A mirror reflects what the camera cannot see, because a reflection traces the whole scene in
 * world space. The red quad is above the frame, and the mirror tilted forty five degrees under
 * it turns every ray up into it. shinymetal with its ambient and its highlight off is Cs times
 * what it traces.
 **/
BOOST_AUTO_TEST_CASE(moya_ribhandler_a_mirror_traces_test) {
    v3d::moya::Renderer renderer;
    v3d::moya::RIBHandler handler(&renderer);

    BOOST_REQUIRE(read(
        "Format 64 48 1\n"
        "PixelSamples 1 1\n"
        "PixelFilter \"box\" 1 1\n"
        "Projection \"perspective\" \"fov\" [30]\n"
        "Clipping 0.1 100\n"
        "WorldBegin\n"
        "AttributeBegin\n"
        "Surface \"shinymetal\" \"Ka\" [0] \"Ks\" [0] \"Kr\" [1]\n"
        "Polygon \"P\" [-1 -1 4  1 -1 4  1 1 6  -1 1 6]\n"
        "AttributeEnd\n"
        "AttributeBegin\n"
        "Color [1 0 0]\n"
        "Surface \"constant\"\n"
        "Polygon \"P\" [-3 3 2  3 3 2  3 3 8  -3 3 8]\n"
        "AttributeEnd\n"
        "WorldEnd\n", &handler));

    const boost::shared_ptr<v3d::render::offline::FrameBuffer> planes = handler.context().picture();
    BOOST_CHECK_CLOSE(planes->value(v3d::moya::Planes::RED, 32, 24), 1.0F, 0.01F);
    BOOST_CHECK_SMALL(planes->value(v3d::moya::Planes::GREEN, 32, 24), 1.0e-6F);
    BOOST_CHECK_SMALL(planes->value(v3d::moya::Planes::BLUE, 32, 24), 1.0e-6F);
}

/**
 * The ray hider does not draw a sphere whose radius is not positive. The sphere is logged and
 * left out of the traced scene. A sphere with a positive radius in the same scene is drawn.
 **/
BOOST_AUTO_TEST_CASE(moya_ribhandler_a_sphere_with_no_size_is_skipped_test) {
    v3d::moya::Renderer renderer;
    v3d::moya::RIBHandler handler(&renderer);

    BOOST_REQUIRE(read(
        "Hider \"raytrace\"\n"
        "Format 16 16 1\n"
        "Projection \"orthographic\"\n"
        "Clipping 1 100\n"
        "WorldBegin\n"
        "Sphere -1 -1 1 360\n"
        "Sphere 0 -1 1 360\n"
        "Sphere 1 -1 1 360\n"
        "WorldEnd\n", &handler));

    BOOST_CHECK_EQUAL(handler.context().traced().all<v3d::render::offline::trace::Sphere>().size(), 1U);
}

/**
 * A rotated polygon is bounded by all eight corners of its object space bound. Its two
 * diagonal corners land at x = 1.5, off the right of the screen window. The quad reaches in
 * to x = 0.79. A bound built from those two corners alone culls a quad that is on screen.
 **/
BOOST_AUTO_TEST_CASE(moya_ribhandler_rotated_polygon_is_not_culled_test) {
    v3d::moya::Renderer renderer;
    v3d::moya::RIBHandler handler(&renderer);

    BOOST_REQUIRE(read(
        "Format 64 48 1\n"
        "PixelSamples 1 1\n"
        "PixelFilter \"box\" 1 1\n"
        "Hider \"hidden\"\n"
        "Projection \"orthographic\"\n"
        "Clipping 1 100\n"
        "WorldBegin\n"
        "Surface \"constant\"\n"
        "Translate 1.5 0 0\n"
        "Rotate 45 0 0 1\n"
        "Polygon \"P\" [0 0 5  1 0 5  1 1 5  0 1 5]\n"
        "WorldEnd\n", &handler));

    const boost::shared_ptr<v3d::render::offline::FrameBuffer> planes = handler.context().picture();
    BOOST_CHECK_GT(coverage(*planes), 0U);
    // the centre of column 56, row 7 is near x = 1.02 and y = 0.69, which is inside the quad
    BOOST_CHECK_GT(planes->value(v3d::moya::Planes::RED, 56, 7), 0.5F);
}

/**
 * A polygon whose first two vertices coincide still splits. Its cutting plane comes from the
 * first edge with a length, and a polygon too large to dice is drawn rather than lost.
 **/
BOOST_AUTO_TEST_CASE(moya_ribhandler_repeated_head_polygon_splits_test) {
    v3d::moya::Renderer renderer;
    v3d::moya::RIBHandler handler(&renderer);

    BOOST_REQUIRE(read(
        "Format 64 48 1\n"
        "Projection \"orthographic\"\n"
        "Clipping 1 100\n"
        "WorldBegin\n"
        "Polygon \"P\" [-1 -1 5  -1 -1 5  1 -1 5  1 1 5  -1 1 5]\n"
        "WorldEnd\n", &handler));

    const boost::shared_ptr<v3d::render::offline::FrameBuffer> planes = handler.context().picture();
    BOOST_CHECK_GT(coverage(*planes), 0U);
    BOOST_CHECK_GT(planes->value(v3d::moya::Planes::RED, 32, 24), 0.5F);
}

/**
 * A split carries each corner's "Cs" onto its pieces, interpolated where it cuts an edge. A
 * quad with a different colour at each corner comes out the same split as diced whole.
 * Colour is bilinear across a grid, and a split of a rectangle through its middle keeps it
 * bilinear. The two pictures differ only by rounding.
 **/
BOOST_AUTO_TEST_CASE(moya_ribhandler_split_carries_colour_test) {
    const std::string shading = "Surface \"constant\"\n";
    const std::string colours = "\"Cs\" [1 0 0  1 1 1  0 0 1  0 1 0]\n";
    const boost::shared_ptr<v3d::render::offline::FrameBuffer> split = quad(16, shading, colours);
    const boost::shared_ptr<v3d::render::offline::FrameBuffer> whole = quad(4096, shading, colours);

    for (const auto & pixel : INSIDE) {
        BOOST_TEST_CONTEXT("pixel " << pixel[0] << ", " << pixel[1]) {
            for (unsigned int channel = 0; channel < 3; channel++) {
                BOOST_CHECK_SMALL(split->value(channel, pixel[0], pixel[1]) - whole->value(channel, pixel[0], pixel[1]),
                    0.03F);
            }
        }
    }
    // the colours vary across the quad, so the comparison is not of two flat pictures. The
    // lower left corner is red and the upper right blue
    BOOST_TEST(split->value(v3d::moya::Planes::RED, 14, 40) > 0.8F);
    BOOST_TEST(split->value(v3d::moya::Planes::BLUE, 14, 40) < 0.2F);
    BOOST_TEST(split->value(v3d::moya::Planes::BLUE, 50, 8) > 0.8F);
    BOOST_TEST(split->value(v3d::moya::Planes::RED, 50, 8) < 0.2F);
}

/**
 * A split carries each corner's "N" onto its pieces too. A quad lit through normals that
 * sweep across it shades as a gradient whether it is split or not. A piece without the
 * normals would shade with the quad's plane, and be lit full on.
 *
 * The normals are interpolated and renormalised at each cut. That differs slightly from the
 * bilinear blend a whole grid makes, so the two pictures agree to a few hundredths.
 **/
BOOST_AUTO_TEST_CASE(moya_ribhandler_split_carries_normal_test) {
    const std::string shading =
        "LightSource \"distantlight\" 1 \"to\" [0 0 1]\n"
        "Surface \"matte\" \"Ka\" [0]\n";
    const std::string normals = "\"N\" [-0.9 0 -0.436  -0.9 0 -0.436  0 0 -1  0 0 -1]\n";
    const boost::shared_ptr<v3d::render::offline::FrameBuffer> split = quad(16, shading, normals);
    const boost::shared_ptr<v3d::render::offline::FrameBuffer> whole = quad(4096, shading, normals);

    for (const auto & pixel : INSIDE) {
        BOOST_TEST_CONTEXT("pixel " << pixel[0] << ", " << pixel[1]) {
            BOOST_CHECK_SMALL(split->value(v3d::moya::Planes::RED, pixel[0], pixel[1]) -
                whole->value(v3d::moya::Planes::RED, pixel[0], pixel[1]), 0.04F);
        }
    }
    // the left edge leans away from the light and the right faces it
    BOOST_TEST(split->value(v3d::moya::Planes::RED, 14, 24) < 0.7F);
    BOOST_TEST(split->value(v3d::moya::Planes::RED, 50, 24) > 0.9F);
}

/**
 * A primitive's bound grows by its displacementbound before it is culled. A quad just past the
 * far clipping plane is culled, unless a bound lets its displacement reach in front of the
 * plane, and the displacement moves it there and it is drawn.
 *
 * The bound is about 12 pixels on each side here, wider than a grid of 16 pixels with the quad.
 * The quad is still diced, because it is measured for splitting without the bound.
 **/
BOOST_AUTO_TEST_CASE(moya_ribhandler_a_displaced_bound_is_not_culled_test) {
    const auto render = [](const std::string & bound) {
        v3d::moya::Renderer renderer;
        v3d::moya::RIBHandler handler(&renderer);
        BOOST_REQUIRE(read(
            "Format 64 48 1\n"
            "PixelSamples 1 1\n"
            "PixelFilter \"box\" 1 1\n"
            "Projection \"orthographic\"\n"
            "Clipping 1 100\n"
            "Option \"searchpath\" \"shader\" [\"data:&\"]\n"
            "WorldBegin\n"
            "Surface \"constant\"\n" + bound +
            "Displacement \"lift\" \"float amount\" [0.5]\n"
            "Polygon \"P\" [-0.5 -0.5 100.2  -0.5 0.5 100.2  0.5 0.5 100.2  0.5 -0.5 100.2]\n"
            "WorldEnd\n", &handler));
        return coverage(*handler.context().picture());
    };
    BOOST_CHECK_EQUAL(render(""), 0U);
    BOOST_CHECK_GT(render("Attribute \"displacementbound\" \"float sphere\" [0.5]\n"), 0U);
}

/**
 * The traced scene holds a displaced surface where its displacement moved it, under either
 * hider, so its shadows follow the moved shape. A floor at y = 0 lifted by one unit blocks a
 * shadow ray from half a unit up, which its undisplaced shape would not. A ray from above the
 * lifted floor meets it one unit up. The light casts shadows, so the reyes hider keeps the
 * traced scene after rendering.
 **/
BOOST_AUTO_TEST_CASE(moya_ribhandler_a_displaced_shadow_follows_the_moved_shape_test) {
    for (const char* hider : { "hidden", "raytrace" }) {
        BOOST_TEST_CONTEXT("under " << hider) {
            v3d::moya::Renderer renderer;
            v3d::moya::RIBHandler handler(&renderer);
            BOOST_REQUIRE(read(
                "Format 16 16 1\n"
                "Hider \"" + std::string(hider) + "\"\n"
                "Projection \"orthographic\"\n"
                "Clipping 0.1 100\n"
                "Option \"searchpath\" \"shader\" [\"data:&\"]\n"
                "Translate 0 0 5\n"
                "WorldBegin\n"
                "LightSource \"distantlight\" 1\n"
                "Attribute \"displacementbound\" \"float sphere\" [1]\n"
                "Displacement \"lift\" \"float amount\" [1]\n"
                "Polygon \"P\" [-2 0 -2  -2 0 2  2 0 2  2 0 -2]\n"
                "WorldEnd\n", &handler));
            const v3d::render::offline::trace::Scene & traced = handler.context().traced();
            BOOST_REQUIRE(!traced.primitives().empty());
            v3d::render::offline::trace::Hit hit;
            const v3d::type::geometry::Ray up({.origin = glm::vec3(0.3F, 0.5F, 0.2F), .direction = glm::vec3(0.0F, 1.0F, 0.0F)});
            BOOST_REQUIRE(traced.nearest(up, 0.0F, &hit));
            BOOST_CHECK_CLOSE(hit.distance, 0.5F, 0.1F);
            const v3d::type::geometry::Ray down({.origin = glm::vec3(0.3F, 3.0F, 0.2F), .direction = glm::vec3(0.0F, -1.0F, 0.0F)});
            BOOST_REQUIRE(traced.nearest(down, 0.0F, &hit));
            BOOST_CHECK_CLOSE(hit.distance, 2.0F, 0.1F);
        }
    }
}

/**
 * A polygon sent before WorldBegin has no camera to be placed by, so it is logged and dropped.
 **/
BOOST_AUTO_TEST_CASE(moya_ribhandler_a_polygon_before_the_world_is_dropped_test) {
    v3d::moya::Renderer renderer;
    v3d::moya::RIBHandler handler(&renderer);
    const Heard heard(handler.context().logger()->get());

    BOOST_REQUIRE(read(
        "Format 64 48 1\n"
        "Polygon \"P\" [-0.2 -0.2 5  0.2 -0.2 5  0.2 0.2 5  -0.2 0.2 5]\n"
        "WorldBegin\n"
        "WorldEnd\n", &handler));

    BOOST_CHECK_EQUAL(handler.context().bucketed(), 0U);
    BOOST_TEST(heard.said("before WorldBegin"));
}

/**
 * A hexagon small enough to be diced whole. Dicing interpolates four corners, so a polygon with
 * more is broken into pieces of four corners or fewer first. Without that, the grid covers only
 * the first four corners, the hexagon's upper half, and the lower half is not drawn.
 **/
BOOST_AUTO_TEST_CASE(moya_ribhandler_a_hexagon_is_diced_whole_test) {
    v3d::moya::Renderer renderer;
    v3d::moya::RIBHandler handler(&renderer);
    BOOST_REQUIRE(read(
        "Option \"limits\" \"gridsize\" [4096]\n"
        "Format 64 48 1\n"
        "PixelSamples 1 1\n"
        "PixelFilter \"box\" 1 1\n"
        "Projection \"orthographic\"\n"
        "Clipping 1 100\n"
        "WorldBegin\n"
        "Color [1 0 0]\n"
        "Polygon \"P\" [0.8 0 5  0.4 0.693 5  -0.4 0.693 5  -0.8 0 5  -0.4 -0.693 5  0.4 -0.693 5]\n"
        "WorldEnd\n", &handler));
    const boost::shared_ptr<v3d::render::offline::FrameBuffer> planes = handler.context().picture();

    // y = 0.4 and y = -0.4 in screen space fall on rows 14 and 33, both inside the hexagon
    BOOST_CHECK_GT(planes->value(v3d::moya::Planes::RED, 32, 14), 0.5F);
    BOOST_CHECK_GT(planes->value(v3d::moya::Planes::RED, 32, 33), 0.5F);
}

};  // namespace
};  // namespace v3d::moya::handling
