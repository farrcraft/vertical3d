/**
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 **/

#include <api/render/offline/rib/Reader.h>
#include <api/render/offline/trace/Sphere.h>
#include <moya/libmoya/RIBHandler.h>

#include <sstream>
#include <string>

#include <boost/test/unit_test.hpp>
#include <boost/make_shared.hpp>

#include <glm/glm.hpp>

namespace {

bool read(const std::string & source, v3d::moya::RIBHandler * handler) {
    v3d::render::offline::rib::Reader reader(boost::make_shared<v3d::log::Logger>());
    std::istringstream stream(source);
    return reader.read(stream, handler);
}

};  // namespace

/**
 * A scene reaches the render context through the handler, so a RIB file drives the offline
 * renderer.
 **/
BOOST_AUTO_TEST_CASE(moya_ribhandler_camera_test) {
    v3d::moya::Renderer renderer;
    v3d::moya::RIBHandler handler(&renderer);

    BOOST_REQUIRE(read(
        "Format 64 48 1\n"
        "Projection \"orthographic\"\n"
        "Clipping 1 100\n"
        "WorldBegin\n"
        "WorldEnd\n", &handler));

    BOOST_CHECK_EQUAL(handler.context().imageWidth(), 64u);
    BOOST_CHECK_EQUAL(handler.context().imageHeight(), 48u);
    BOOST_REQUIRE(handler.context().framebuffer());
    BOOST_CHECK_EQUAL(handler.context().framebuffer()->planes()->width(), 64u);
}

/**
 * The sampling requests reach the render context, and a scene that names none of them is
 * sampled at the RI defaults.
 **/
BOOST_AUTO_TEST_CASE(moya_ribhandler_sampling_test) {
    v3d::moya::Renderer renderer;
    v3d::moya::RIBHandler handler(&renderer);

    BOOST_REQUIRE(read(
        "Format 64 48 1\n"
        "PixelSamples 4 4\n"
        "DepthOfField 8 0.1 3\n"
        "WorldBegin\n"
        "WorldEnd\n", &handler));

    BOOST_CHECK_EQUAL(handler.context().sampling().samples.x, 4u);
    BOOST_CHECK_EQUAL(handler.context().sampling().samples.y, 4u);
    BOOST_CHECK_EQUAL(handler.context().sampling().fstop, 8.0f);
    BOOST_CHECK_CLOSE(handler.context().sampling().focalLength, 0.1f, 1.0e-4f);
    BOOST_CHECK_EQUAL(handler.context().sampling().focalDistance, 3.0f);

    v3d::moya::Renderer plainRenderer;
    v3d::moya::RIBHandler silent(&plainRenderer);
    BOOST_REQUIRE(read("Format 64 48 1\nWorldBegin\nWorldEnd\n", &silent));
    BOOST_CHECK_EQUAL(silent.context().sampling().samples.x, 2u);
    BOOST_CHECK_EQUAL(silent.context().sampling().samples.y, 2u);
    BOOST_CHECK(silent.context().sampling().filter == v3d::render::offline::Filter::Gaussian);
    BOOST_CHECK_EQUAL(silent.context().sampling().width.x, 2.0f);
    BOOST_CHECK(silent.context().sampling().pinhole());
}

/**
 * The polygon a file names reaches the first pass with the points it named, moved into eye
 * space by the transform that was current.
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

    BOOST_CHECK_EQUAL(handler.context().framebuffer()->primitiveCount(), 1u);
}

/**
 * A scene places its camera with a matrix that translates, as the standard's own example
 * does. The world to camera transformation applies as it stands: a transpose and an
 * inverse are both right only when it is a rotation, and neither is when it is not.
 **/
BOOST_AUTO_TEST_CASE(moya_ribhandler_camera_transform_test) {
    v3d::moya::Renderer renderer;
    v3d::moya::RIBHandler handler(&renderer);

    // the camera sits four units back along -z, so a quad at the world origin is four units
    // in front of it and inside the [1, 100] clip range
    BOOST_REQUIRE(read(
        "Format 64 48 1\n"
        "Projection \"orthographic\"\n"
        "Clipping 1 100\n"
        "Transform [1 0 0 0  0 1 0 0  0 0 1 0  0 0 4 1]\n"
        "WorldBegin\n"
        "Polygon \"P\" [-0.2 -0.2 0  0.2 -0.2 0  0.2 0.2 0  -0.2 0.2 0]\n"
        "WorldEnd\n", &handler));

    // it survived the hither-yon cull, which it could not have at a depth of zero
    BOOST_CHECK_EQUAL(handler.context().framebuffer()->primitiveCount(), 1u);
}

/**
 * RiColor is graphics state rather than a material, and it reaches the pixels. Without it
 * every scene is one flat grey, which is a picture that cannot tell two polygons apart.
 **/
BOOST_AUTO_TEST_CASE(moya_ribhandler_color_test) {
    v3d::moya::Renderer renderer;
    v3d::moya::RIBHandler handler(&renderer);

    BOOST_REQUIRE(read(
        "Format 64 48 1\n"
        "Projection \"orthographic\"\n"
        "Clipping 1 100\n"
        "WorldBegin\n"
        "Color [1 0 0]\n"
        "Polygon \"P\" [-0.2 -0.2 5  0.2 -0.2 5  0.2 0.2 5  -0.2 0.2 5]\n"
        "WorldEnd\n", &handler));

    boost::shared_ptr<v3d::render::offline::FrameBuffer> planes = handler.context().framebuffer()->planes();
    BOOST_CHECK_EQUAL(planes->value(v3d::moya::FrameBuffer::RED, 32, 24), 1.0f);
    BOOST_CHECK_EQUAL(planes->value(v3d::moya::FrameBuffer::GREEN, 32, 24), 0.0f);
    BOOST_CHECK_EQUAL(planes->value(v3d::moya::FrameBuffer::BLUE, 32, 24), 0.0f);
}

/**
 * The current colour and transform push and pop together, so the second polygon is not
 * coloured or placed by what the first one set.
 **/
BOOST_AUTO_TEST_CASE(moya_ribhandler_attribute_block_test) {
    v3d::moya::Renderer renderer;
    v3d::moya::RIBHandler handler(&renderer);

    BOOST_REQUIRE(read(
        "Format 64 48 1\n"
        "Projection \"orthographic\"\n"
        "Clipping 1 100\n"
        "WorldBegin\n"
        "AttributeBegin\n"
        "Color [1 0 0]\n"
        "Translate -0.5 0 5\n"
        "Polygon \"P\" [-0.2 -0.2 0  0.2 -0.2 0  0.2 0.2 0  -0.2 0.2 0]\n"
        "AttributeEnd\n"
        "AttributeBegin\n"
        "Color [0 0 1]\n"
        "Translate 0.5 0 5\n"
        "Polygon \"P\" [-0.2 -0.2 0  0.2 -0.2 0  0.2 0.2 0  -0.2 0.2 0]\n"
        "AttributeEnd\n"
        "WorldEnd\n", &handler));

    boost::shared_ptr<v3d::render::offline::FrameBuffer> planes = handler.context().framebuffer()->planes();
    // the screen window is [-4/3, 4/3] by [-1, 1] on a 64 by 48 frame, so -0.5 in x lands at
    // column 20 and +0.5 at column 44
    BOOST_CHECK_EQUAL(planes->value(v3d::moya::FrameBuffer::RED, 20, 24), 1.0f);
    BOOST_CHECK_EQUAL(planes->value(v3d::moya::FrameBuffer::BLUE, 20, 24), 0.0f);
    BOOST_CHECK_EQUAL(planes->value(v3d::moya::FrameBuffer::BLUE, 44, 24), 1.0f);
    BOOST_CHECK_EQUAL(planes->value(v3d::moya::FrameBuffer::RED, 44, 24), 0.0f);
}

/**
 * A perspective projection makes the nearer of two equal quads the larger one. An identity
 * projection would make them the same size.
 **/
BOOST_AUTO_TEST_CASE(moya_ribhandler_perspective_test) {
    v3d::moya::Renderer renderer;
    v3d::moya::RIBHandler handler(&renderer);

    BOOST_REQUIRE(read(
        "Format 64 48 1\n"
        "Projection \"perspective\" \"fov\" 45\n"
        "Clipping 1 100\n"
        "WorldBegin\n"
        "Color [1 0 0]\n"
        "Polygon \"P\" [-0.4 -0.4 4  0.4 -0.4 4  0.4 0.4 4  -0.4 0.4 4]\n"
        "WorldEnd\n", &handler));

    boost::shared_ptr<v3d::render::offline::FrameBuffer> planes = handler.context().framebuffer()->planes();

    unsigned int covered = 0;
    for (unsigned int row = 0; row < 48; row++) {
        for (unsigned int column = 0; column < 64; column++) {
            if (planes->value(v3d::moya::FrameBuffer::RED, column, row) > 0.5f) {
                covered++;
            }
        }
    }
    BOOST_CHECK_GT(covered, 0u);

    // the same quad twice as far away covers a quarter of the pixels under a perspective
    // projection, and the same pixels under an identity matrix
    v3d::moya::Renderer far;
    v3d::moya::RIBHandler distant(&far);
    BOOST_REQUIRE(read(
        "Format 64 48 1\n"
        "Projection \"perspective\" \"fov\" 45\n"
        "Clipping 1 100\n"
        "WorldBegin\n"
        "Color [1 0 0]\n"
        "Polygon \"P\" [-0.4 -0.4 8  0.4 -0.4 8  0.4 0.4 8  -0.4 0.4 8]\n"
        "WorldEnd\n", &distant));

    boost::shared_ptr<v3d::render::offline::FrameBuffer> distantPlanes = distant.context().framebuffer()->planes();
    unsigned int distantCovered = 0;
    for (unsigned int row = 0; row < 48; row++) {
        for (unsigned int column = 0; column < 64; column++) {
            if (distantPlanes->value(v3d::moya::FrameBuffer::RED, column, row) > 0.5f) {
                distantCovered++;
            }
        }
    }
    BOOST_CHECK_GT(distantCovered, 0u);
    BOOST_CHECK_CLOSE(static_cast<float>(covered) / static_cast<float>(distantCovered), 4.0f, 15.0f);
}

/**
 * The screen window follows the frame aspect, so a frame that is not square does not stretch
 * a square window across itself: a quad as wide as it is tall covers as many columns as rows.
 **/
BOOST_AUTO_TEST_CASE(moya_ribhandler_screen_window_test) {
    v3d::moya::Renderer renderer;
    v3d::moya::RIBHandler handler(&renderer);

    BOOST_REQUIRE(read(
        "Format 64 48 1\n"
        "Projection \"orthographic\"\n"
        "Clipping 1 100\n"
        "WorldBegin\n"
        "Color [1 0 0]\n"
        "Polygon \"P\" [-0.5 -0.5 5  0.5 -0.5 5  0.5 0.5 5  -0.5 0.5 5]\n"
        "WorldEnd\n", &handler));

    boost::shared_ptr<v3d::render::offline::FrameBuffer> planes = handler.context().framebuffer()->planes();

    unsigned int columns = 0;
    for (unsigned int column = 0; column < 64; column++) {
        if (planes->value(v3d::moya::FrameBuffer::RED, column, 24) > 0.5f) {
            columns++;
        }
    }
    unsigned int rows = 0;
    for (unsigned int row = 0; row < 48; row++) {
        if (planes->value(v3d::moya::FrameBuffer::RED, 32, row) > 0.5f) {
            rows++;
        }
    }

    BOOST_CHECK_GT(columns, 0u);
    // one pixel of slack for where the edges land against the sample centres
    BOOST_CHECK_LE(columns > rows ? columns - rows : rows - columns, 1u);
}

/**
 * An explicit screen window stops the frame aspect writing one, and RiFormat afterwards does
 * not put it back.
 **/
BOOST_AUTO_TEST_CASE(moya_ribhandler_explicit_screen_window_test) {
    v3d::moya::Renderer renderer;
    v3d::moya::RIBHandler handler(&renderer);

    BOOST_REQUIRE(read(
        "ScreenWindow -2 2 -2 2\n"
        "Format 64 48 1\n"
        "PixelSamples 1 1\n"
        "PixelFilter \"box\" 1 1\n"
        "Projection \"orthographic\"\n"
        "Clipping 1 100\n"
        "WorldBegin\n"
        "Color [1 0 0]\n"
        "Polygon \"P\" [-1.9 -1.9 5  1.9 -1.9 5  1.9 1.9 5  -1.9 1.9 5]\n"
        "WorldEnd\n", &handler));

    boost::shared_ptr<v3d::render::offline::FrameBuffer> planes = handler.context().framebuffer()->planes();
    // a window of [-2, 2] means the quad nearly fills the frame; the default [-4/3, 4/3] by
    // [-1, 1] would have clipped it away at the top and bottom
    BOOST_CHECK_EQUAL(planes->value(v3d::moya::FrameBuffer::RED, 32, 1), 1.0f);
    BOOST_CHECK_EQUAL(planes->value(v3d::moya::FrameBuffer::RED, 32, 46), 1.0f);
}

/**
 * Option "limits" is how a scene names the bucket and grid sizes. The standard's example file
 * opens with it.
 **/
BOOST_AUTO_TEST_CASE(moya_ribhandler_limits_test) {
    v3d::moya::Renderer renderer;
    v3d::moya::RIBHandler handler(&renderer);

    BOOST_REQUIRE(read(
        "Option \"limits\" \"bucketsize\" [6 6]\n"
        "Option \"limits\" \"gridsize\" [18]\n", &handler));

    BOOST_CHECK_EQUAL(handler.context().bucketWidth(), 6u);
    BOOST_CHECK_EQUAL(handler.context().bucketHeight(), 6u);
    BOOST_CHECK_EQUAL(handler.context().gridSize(), 18u);
}

/**
 * TransformEnd restores what TransformBegin saved, rather than leaving the current
 * transformation wherever the block moved it.
 **/
BOOST_AUTO_TEST_CASE(moya_ribhandler_transform_block_test) {
    v3d::moya::Renderer renderer;
    v3d::moya::RIBHandler handler(&renderer);

    // the block leaves nothing behind: a polygon added after it sits where its own points say
    BOOST_REQUIRE(read(
        "Format 64 48 1\n"
        "Projection \"orthographic\"\n"
        "Clipping 1 100\n"
        "WorldBegin\n"
        "TransformBegin\n"
        "Translate 5 5 5\n"
        "TransformEnd\n"
        "Color [1 0 0]\n"
        "Polygon \"P\" [-0.2 -0.2 5  0.2 -0.2 5  0.2 0.2 5  -0.2 0.2 5]\n"
        "WorldEnd\n", &handler));

    boost::shared_ptr<v3d::render::offline::FrameBuffer> planes = handler.context().framebuffer()->planes();
    BOOST_CHECK_EQUAL(planes->value(v3d::moya::FrameBuffer::RED, 32, 24), 1.0f);
}

/**
 * The driver's --output means that file whatever the scene's Display names.
 **/
BOOST_AUTO_TEST_CASE(moya_ribhandler_output_override_test) {
    v3d::moya::Renderer renderer;
    v3d::moya::RIBHandler handler(&renderer);
    handler.output("data_out/override.png");

    BOOST_REQUIRE(read("Display \"scene-says.png\" \"file\" \"rgb\"\n", &handler));

    // nothing is written until WorldEnd, so this reads the context rather than the disk
    BOOST_CHECK_EQUAL(handler.context().displayName(), "data_out/override.png");
}

/**
 * The reyes hider dices polygons only, so a scene with a sphere in it renders without the
 * sphere and says so, rather than failing. The ray hider draws one.
 **/
BOOST_AUTO_TEST_CASE(moya_ribhandler_sphere_is_skipped_test) {
    v3d::moya::Renderer renderer;
    v3d::moya::RIBHandler handler(&renderer);

    BOOST_REQUIRE(read(
        "Format 64 48 1\n"
        "Projection \"orthographic\"\n"
        "Clipping 1 100\n"
        "WorldBegin\n"
        "Sphere 1 -1 1 360\n"
        "Polygon \"P\" [-0.2 -0.2 5  0.2 -0.2 5  0.2 0.2 5  -0.2 0.2 5]\n"
        "WorldEnd\n", &handler));

    BOOST_CHECK_EQUAL(handler.context().framebuffer()->primitiveCount(), 1u);
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

    boost::shared_ptr<v3d::render::offline::FrameBuffer> planes = handler.context().framebuffer()->planes();
    BOOST_CHECK_CLOSE(planes->value(v3d::moya::FrameBuffer::RED, 32, 24), 1.0f, 0.01f);
    BOOST_CHECK_SMALL(planes->value(v3d::moya::FrameBuffer::GREEN, 32, 24), 1.0e-6f);
    BOOST_CHECK_SMALL(planes->value(v3d::moya::FrameBuffer::BLUE, 32, 24), 1.0e-6f);
}

/**
 * A size given to the handler replaces the size a scene's Format names, as a command line
 * does, while the scene still renders at that size.
 **/
BOOST_AUTO_TEST_CASE(moya_ribhandler_resolution_override_test) {
    v3d::moya::Renderer renderer;
    v3d::moya::RIBHandler handler(&renderer);
    handler.resolution(8, 4);

    BOOST_REQUIRE(read("Format 64 48 1\nWorldBegin\nWorldEnd\n", &handler));

    BOOST_CHECK_EQUAL(handler.context().framebuffer()->planes()->width(), 8u);
    BOOST_CHECK_EQUAL(handler.context().framebuffer()->planes()->height(), 4u);
}

/**
 * A sphere whose radius is not positive is not drawn by the ray hider: it is logged and left
 * out of the traced scene, rather than built with a range that has no meaning. One with a
 * positive radius in the same scene is drawn.
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

    BOOST_CHECK_EQUAL(handler.context().traced().all<v3d::render::offline::trace::Sphere>().size(), 1u);
}

/**
 * A primitive whose motion is flat at both ends has no pose to be moved from. A polygon under
 * such a motion is left out of the frame and a sphere out of the traced scene, and neither
 * throws or leaves a pose that is not a number.
 **/
BOOST_AUTO_TEST_CASE(moya_ribhandler_motion_flat_at_both_ends_test) {
    const std::string flat =
        "Format 16 16 1\n"
        "Shutter 0 1\n"
        "Projection \"orthographic\"\n"
        "Clipping 1 100\n"
        "WorldBegin\n"
        "MotionBegin [0 1]\n"
        "Scale 0 1 1\n"
        "Scale 1 0 1\n"
        "MotionEnd\n";
    {
        v3d::moya::Renderer renderer;
        v3d::moya::RIBHandler handler(&renderer);
        BOOST_REQUIRE(read(flat + "Polygon \"P\" [-0.5 -0.5 2  0.5 -0.5 2  0.5 0.5 2  -0.5 0.5 2]\nWorldEnd\n",
            &handler));
        BOOST_CHECK_EQUAL(handler.context().framebuffer()->primitiveCount(), 0u);
    }
    {
        v3d::moya::Renderer renderer;
        v3d::moya::RIBHandler handler(&renderer);
        BOOST_REQUIRE(read("Hider \"raytrace\"\n" + flat + "Sphere 1 -1 1 360\nWorldEnd\n", &handler));
        BOOST_CHECK(handler.context().traced().all<v3d::render::offline::trace::Sphere>().empty());
    }
}
