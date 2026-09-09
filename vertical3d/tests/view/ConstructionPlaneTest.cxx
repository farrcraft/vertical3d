/**
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 **/

#include <vertical3d/src/view/ConstructionPlane.h>

#include <algorithm>

#include <boost/test/unit_test.hpp>

BOOST_AUTO_TEST_CASE(constructionplane_geometry_test) {
    v3d::editor::ConstructionPlane grid;
    v3d::type::Camera camera;
    v3d::render::realtime::LineCanvas canvas;

    grid.lines(4);
    grid.spacing(1.0f);
    grid.draw(camera, &canvas);

    // one line each way per division, plus the one that closes the square, and two vertices
    // per segment because a line list shares nothing
    BOOST_CHECK_EQUAL(canvas.vertices().size(), 2u * 2u * (4u + 1u));
}

BOOST_AUTO_TEST_CASE(constructionplane_extent_test) {
    v3d::editor::ConstructionPlane grid;
    v3d::type::Camera camera;
    v3d::render::realtime::LineCanvas canvas;

    // the grid is centred on the origin, so a grid four lines across at unit spacing runs
    // from -2 to 2 in both directions
    grid.lines(4);
    grid.spacing(1.0f);
    grid.draw(camera, &canvas);

    float minU = 0.0f;
    float maxU = 0.0f;
    float minV = 0.0f;
    float maxV = 0.0f;
    for (const v3d::render::realtime::LineCanvas::Vertex& vertex : canvas.vertices()) {
        minU = std::min(minU, vertex.position.x);
        maxU = std::max(maxU, vertex.position.x);
        minV = std::min(minV, vertex.position.y);
        maxV = std::max(maxV, vertex.position.y);
    }
    BOOST_CHECK_CLOSE(minU, -2.0f, 0.01f);
    BOOST_CHECK_CLOSE(maxU, 2.0f, 0.01f);
    // a default camera is orthographic and looks along +z, so its own right and up are x
    // and y - the grid lies in the plane the view faces
    BOOST_CHECK_CLOSE(minV, -2.0f, 0.01f);
    BOOST_CHECK_CLOSE(maxV, 2.0f, 0.01f);
}

BOOST_AUTO_TEST_CASE(constructionplane_perspective_plane_test) {
    v3d::editor::ConstructionPlane grid;
    v3d::type::Camera camera;
    v3d::render::realtime::LineCanvas canvas;

    // a perspective view gets the ground plane instead, so nothing leaves y = 0
    camera.orthographic(false);
    grid.lines(2);
    grid.draw(camera, &canvas);

    BOOST_REQUIRE(!canvas.empty());
    for (const v3d::render::realtime::LineCanvas::Vertex& vertex : canvas.vertices()) {
        BOOST_CHECK_SMALL(vertex.position.y, 0.001f);
    }
}

BOOST_AUTO_TEST_CASE(constructionplane_emphasis_test) {
    v3d::editor::ConstructionPlane grid;
    v3d::type::Camera camera;
    v3d::render::realtime::LineCanvas canvas;

    // the two lines through the origin are drawn in the origin colour, the ones on a major
    // interval in the major colour, and everything else in the minor one
    grid.lines(4);
    grid.intervals(2);
    grid.spacing(1.0f);
    grid.draw(camera, &canvas);

    unsigned int distinct = 0;
    glm::vec4 seen(-1.0f);
    for (const v3d::render::realtime::LineCanvas::Vertex& vertex : canvas.vertices()) {
        if (vertex.colour != seen) {
            distinct++;
            seen = vertex.colour;
        }
    }
    BOOST_CHECK_GT(distinct, 1u);
}

BOOST_AUTO_TEST_CASE(constructionplane_degenerate_test) {
    v3d::editor::ConstructionPlane grid;
    v3d::type::Camera camera;
    v3d::render::realtime::LineCanvas canvas;

    // a grid of no lines, or one whose lines are zero apart, is nothing rather than an
    // infinite loop or a single overdrawn line
    grid.lines(0);
    grid.draw(camera, &canvas);
    BOOST_CHECK(canvas.empty());

    grid.lines(10);
    grid.spacing(0.0f);
    grid.draw(camera, &canvas);
    BOOST_CHECK(canvas.empty());

    // and a null canvas is not dereferenced
    grid.spacing(1.0f);
    grid.draw(camera, nullptr);
}
