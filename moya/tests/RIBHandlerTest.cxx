/**
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 **/

#include <sstream>
#include <string>

#include <boost/test/unit_test.hpp>
#include <boost/make_shared.hpp>

#include <glm/glm.hpp>

#include "../libmoya/RIBHandler.h"

#include "../../api/render/offline/RIBReader.h"

namespace {

    bool read(const std::string & source, v3d::moya::RIBHandler * handler) {
        v3d::render::offline::RIBReader reader(boost::make_shared<v3d::log::Logger>());
        std::istringstream stream(source);
        return reader.read(stream, handler);
    }

};  // namespace

/**
 * A scene reaches the render context through the handler, which is the whole of ADR-0023's
 * claim that one reader drives both renderers.
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
 * A scene places its camera with a matrix that translates, which is what the standard's own
 * example does. The world to camera transformation applies as it stands: a transpose and an
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
 * A perspective projection makes the nearer of two equal quads the larger one. Until this
 * phase RiProjection("perspective") built the identity matrix, so it made them the same size.
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

    // the same quad twice as far away covers a quarter of the pixels, which is what a
    // perspective projection means and what an identity matrix would not do
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
 * Option "limits" is how a scene names the bucket and grid sizes, which is what the standard's
 * example file opens with.
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
 * TransformEnd restores what TransformBegin saved. Popping without restoring left the current
 * transformation wherever the block had moved it.
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
