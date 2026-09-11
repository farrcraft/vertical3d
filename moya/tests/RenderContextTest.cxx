/**
 * Vertical3D
 * Copyright(c) 2022 Joshua Farr(josh@farrcraft.com)
 **/

#include <moya/libmoya/RenderContext.h>

#include <string>
#include <vector>

#include <boost/test/unit_test.hpp>
#include <boost/make_shared.hpp>

#include <glm/glm.hpp>

namespace {

v3d::moya::Vertex vertex(float x, float y, float z) {
    v3d::moya::Vertex v;
    v.point(glm::vec3(x, y, z));
    return v;
}

};  // namespace

/**
 * The defaults a context starts on, which stand in for the RiFormat and bucketing options a
 * RIB file would otherwise set.
 **/
BOOST_AUTO_TEST_CASE(render_context_defaults_test) {
    v3d::moya::RenderContext rc;

    BOOST_TEST(rc.bucketWidth() == 16u);
    BOOST_TEST(rc.bucketHeight() == 16u);
    BOOST_TEST(rc.gridSize() == 256u);
    BOOST_TEST(rc.shadingRate() == 1.0f);
    BOOST_TEST(rc.imageWidth() == 320u);
    BOOST_TEST(rc.imageHeight() == 240u);
    BOOST_TEST(rc.pixelAspect() == 1.0f);
}

BOOST_AUTO_TEST_CASE(render_context_image_resolution_test) {
    v3d::moya::RenderContext rc;

    rc.imageResolution(1024, 768, 1.3f);

    BOOST_TEST(rc.imageWidth() == 1024u);
    BOOST_TEST(rc.imageHeight() == 768u);
    BOOST_TEST(rc.pixelAspect() == 1.3f);
}

/**
 * A named context takes the same defaults as an unnamed one - the name is what RiBegin is
 * given and is not itself an option.
 **/
BOOST_AUTO_TEST_CASE(render_context_named_test) {
    v3d::moya::RenderContext rc(std::string("scene.rib"));

    BOOST_TEST(rc.imageWidth() == 320u);
    BOOST_TEST(rc.gridSize() == 256u);
}

/**
 * The six reserved systems are identity until something saves over them, so a lookup before
 * any transform has been set does not hand back an uninitialised matrix.
 **/
BOOST_AUTO_TEST_CASE(render_context_reserved_coordinate_systems_test) {
    v3d::moya::RenderContext rc;
    glm::mat4x4 identity(1.0f);

    const char* reserved[] = { "object", "world", "camera", "screen", "raster", "NDC" };
    for (const auto* name : reserved) {
        BOOST_TEST((rc.coordinateSystem(name) == identity));
    }
}

/**
 * saveCoordinateSystem files the current transform under a name, and setCoordinateSystem is
 * the way back - together they are what RiCoordinateSystem and RiCoordSysTransform do.
 **/
BOOST_AUTO_TEST_CASE(render_context_coordinate_system_test) {
    v3d::moya::RenderContext rc;
    glm::mat4x4 scaled(3.0f);

    rc.setTransform(scaled);
    rc.saveCoordinateSystem("world");
    BOOST_TEST((rc.coordinateSystem("world") == scaled));

    rc.setIdentityTransform();
    rc.saveCoordinateSystem("world");
    BOOST_TEST((rc.coordinateSystem("world") == glm::mat4x4(1.0f)));

    rc.setCoordinateSystem("world");
    rc.saveCoordinateSystem("object");
    BOOST_TEST((rc.coordinateSystem("object") == glm::mat4x4(1.0f)));
}

/**
 * A translate composes onto the current transform rather than replacing it, so two of them
 * accumulate.
 **/
BOOST_AUTO_TEST_CASE(render_context_translate_test) {
    v3d::moya::RenderContext rc;

    rc.setIdentityTransform();
    rc.translate(1.0f, 2.0f, 3.0f);
    rc.translate(1.0f, 2.0f, 3.0f);
    rc.saveCoordinateSystem("object");

    glm::mat4x4 composed = rc.coordinateSystem("object");
    BOOST_TEST((glm::vec3(composed[3]) == glm::vec3(2.0f, 4.0f, 6.0f)));
}

/**
 * A rotate composes onto the current transform the way a translate does. Both it and scale
 * used to have empty bodies, so RiRotate and RiScale were silent no-ops.
 **/
BOOST_AUTO_TEST_CASE(render_context_rotate_test) {
    v3d::moya::RenderContext rc;

    rc.setIdentityTransform();
    rc.rotate(90.0f, 0.0f, 0.0f, 1.0f);
    rc.saveCoordinateSystem("object");

    // the angle is in degrees, which is what RiRotate states it in
    glm::mat4x4 composed = rc.coordinateSystem("object");
    glm::vec3 turned = glm::vec3(composed * glm::vec4(1.0f, 0.0f, 0.0f, 1.0f));
    BOOST_TEST(turned.x == 0.0f, boost::test_tools::tolerance(0.0001f));
    BOOST_TEST(turned.y == 1.0f, boost::test_tools::tolerance(0.0001f));
}

BOOST_AUTO_TEST_CASE(render_context_scale_test) {
    v3d::moya::RenderContext rc;

    rc.setIdentityTransform();
    rc.scale(2.0f, 3.0f, 4.0f);
    rc.saveCoordinateSystem("object");

    glm::mat4x4 composed = rc.coordinateSystem("object");
    glm::vec3 scaled = glm::vec3(composed * glm::vec4(1.0f, 1.0f, 1.0f, 1.0f));
    BOOST_TEST((scaled == glm::vec3(2.0f, 3.0f, 4.0f)));
}

/**
 * The first pass files a polygon in the bucket its raster bound opens in.
 **/
BOOST_AUTO_TEST_CASE(render_context_buckets_a_polygon_test) {
    v3d::moya::RenderContext rc;
    rc.prepareWorld();

    boost::shared_ptr<v3d::moya::Polygon> polygon = boost::make_shared<v3d::moya::Polygon>();
    polygon->addVertex(vertex(-0.9f, -0.9f, 5.0f));
    polygon->addVertex(vertex(0.9f, -0.9f, 5.0f));
    polygon->addVertex(vertex(0.9f, 0.9f, 5.0f));
    polygon->addVertex(vertex(-0.9f, 0.9f, 5.0f));
    rc.addPolygon(polygon);

    BOOST_TEST(rc.framebuffer()->primitiveCount() == 1u);
}

/**
 * A polygon far larger than one grid is undiceable, so the second pass splits it and hands the
 * pieces back to the first, which measures each in turn. The recursion ends because a split
 * that does not shrink its input is not handed back at all - without that the pieces would be
 * re-split forever.
 **/
BOOST_AUTO_TEST_CASE(render_context_split_terminates_test) {
    v3d::moya::RenderContext rc;
    rc.prepareWorld();

    boost::shared_ptr<v3d::moya::Polygon> polygon = boost::make_shared<v3d::moya::Polygon>();
    polygon->addVertex(vertex(-0.9f, -0.9f, 5.0f));
    polygon->addVertex(vertex(0.9f, -0.9f, 5.0f));
    polygon->addVertex(vertex(0.9f, 0.9f, 5.0f));
    polygon->addVertex(vertex(-0.9f, 0.9f, 5.0f));
    rc.addPolygon(polygon);

    rc.render();

    /*
        The default screen window is [-4/3, 4/3] by [-1, 1] over a 320 by 240 image, so the
        raster bound is 216 pixels across. A grid covers 16, so four rounds of four way
        splitting bring every piece under one: 4^4 pieces, all diceable and bucketed. Reaching
        the count at all is half the assertion - a split that failed to shrink its input would
        be re-split without end.
    */
    BOOST_TEST(rc.framebuffer()->primitiveCount() == 256u);
}

namespace {

/**
 * A triangle small enough that the first pass measures it as diceable, so its vertices are
 * moved into eye space rather than handed to the splitter. Its plane is (1, 1, 0)
 * normalised, which is a normal no axis aligned scale leaves alone.
 **/
boost::shared_ptr<v3d::moya::Polygon> diagonal() {
    boost::shared_ptr<v3d::moya::Polygon> polygon = boost::make_shared<v3d::moya::Polygon>();
    polygon->addVertex(vertex(0.01f, 0.0f, 5.0f));
    polygon->addVertex(vertex(0.0f, 0.01f, 5.0f));
    polygon->addVertex(vertex(0.0f, 0.01f, 5.01f));
    return polygon;
}

};  // namespace

/**
 * A polygon that says nothing about its normals gets its own plane on every vertex, as both Ng
 * and N - which is what makes such a surface faceted.
 **/
BOOST_AUTO_TEST_CASE(render_context_face_normal_test) {
    v3d::moya::RenderContext rc;
    rc.prepareWorld();

    boost::shared_ptr<v3d::moya::Polygon> polygon = boost::make_shared<v3d::moya::Polygon>();
    polygon->addVertex(vertex(-0.01f, -0.01f, 5.0f));
    polygon->addVertex(vertex(0.01f, -0.01f, 5.0f));
    polygon->addVertex(vertex(0.01f, 0.01f, 5.0f));
    // nothing has written a shading normal yet, which is what the fill in is for
    BOOST_TEST(!polygon->vertex(0).hasNormal());

    rc.addPolygon(polygon);

    BOOST_REQUIRE(polygon->diceable());
    for (size_t i = 0; i < polygon->vertexCount(); i++) {
        BOOST_TEST((polygon->vertex(i).geometricNormal() == glm::vec3(0.0f, 0.0f, 1.0f)));
        BOOST_TEST((polygon->vertex(i).normal() == glm::vec3(0.0f, 0.0f, 1.0f)));
    }
}

/**
 * A varying "N" is the shading normal and overrides the plane; Ng stays the plane, because SL
 * defines faceforward() and calculatenormal() in terms of the pair.
 **/
BOOST_AUTO_TEST_CASE(render_context_normal_override_test) {
    v3d::moya::RenderContext rc;
    rc.prepareWorld();

    boost::shared_ptr<v3d::moya::Polygon> polygon = boost::make_shared<v3d::moya::Polygon>();
    for (int i = 0; i < 3; i++) {
        v3d::moya::Vertex v = vertex(i == 1 ? 0.01f : -0.01f, i == 2 ? 0.01f : -0.01f, 5.0f);
        v.normal(glm::vec3(0.0f, 1.0f, 0.0f));
        polygon->addVertex(v);
    }
    rc.addPolygon(polygon);

    BOOST_REQUIRE(polygon->diceable());
    for (size_t i = 0; i < polygon->vertexCount(); i++) {
        BOOST_TEST((polygon->vertex(i).normal() == glm::vec3(0.0f, 1.0f, 0.0f)));
        BOOST_TEST((polygon->vertex(i).geometricNormal() == glm::vec3(0.0f, 0.0f, 1.0f)));
    }
}

/**
 * A normal transforms by the inverse transpose. Under a rotation or a uniform scale that is
 * the same matrix that moves the points, so only a scale of one axis tells the two apart: the
 * normal of the transformed plane leans the opposite way to the points.
 **/
BOOST_AUTO_TEST_CASE(render_context_normal_inverse_transpose_test) {
    v3d::moya::RenderContext rc;
    rc.prepareWorld();
    rc.scale(1.0f, 2.0f, 1.0f);

    boost::shared_ptr<v3d::moya::Polygon> polygon = diagonal();
    rc.addPolygon(polygon);

    BOOST_REQUIRE(polygon->diceable());
    // the object plane is (1, 1, 0) normalised; the inverse transpose of a scale of two in y
    // halves that component, which is the plane the doubled points actually lie in
    const glm::vec3 expected = glm::normalize(glm::vec3(1.0f, 0.5f, 0.0f));
    for (size_t i = 0; i < polygon->vertexCount(); i++) {
        const glm::vec3 normal = polygon->vertex(i).geometricNormal();
        BOOST_TEST(normal.x == expected.x, boost::test_tools::tolerance(0.0001f));
        BOOST_TEST(normal.y == expected.y, boost::test_tools::tolerance(0.0001f));
        BOOST_TEST(normal.z == expected.z, boost::test_tools::tolerance(0.0001f));
    }

    // and the transformed vertices agree: the plane the normal names is the plane they are in
    const glm::vec3 a = polygon->vertex(0).point();
    const glm::vec3 b = polygon->vertex(1).point();
    const glm::vec3 c = polygon->vertex(2).point();
    const glm::vec3 measured = glm::normalize(glm::cross(b - a, c - a));
    BOOST_TEST(measured.x == expected.x, boost::test_tools::tolerance(0.0001f));
    BOOST_TEST(measured.y == expected.y, boost::test_tools::tolerance(0.0001f));
}

/**
 * Dicing interpolates the shading normal onto the grid the way it interpolates the position and
 * the colour, which is what makes a surface given a varying "N" come out smooth. The geometric
 * normal is one value across the primitive and is copied rather than interpolated.
 **/
BOOST_AUTO_TEST_CASE(render_context_dice_interpolates_normal_test) {
    v3d::moya::RenderContext rc;
    rc.prepareWorld();

    boost::shared_ptr<v3d::moya::Polygon> polygon = boost::make_shared<v3d::moya::Polygon>();
    const glm::vec3 corners[4] = {
        glm::vec3(-0.01f, -0.01f, 5.0f), glm::vec3(0.01f, -0.01f, 5.0f),
        glm::vec3(0.01f, 0.01f, 5.0f), glm::vec3(-0.01f, 0.01f, 5.0f)
    };
    for (int i = 0; i < 4; i++) {
        v3d::moya::Vertex v;
        v.point(corners[i]);
        // the first corner leans one way and the rest face front, so a midpoint of the grid
        // has to be between the two rather than either
        v.normal(i == 0 ? glm::normalize(glm::vec3(1.0f, 0.0f, 1.0f)) : glm::vec3(0.0f, 0.0f, 1.0f));
        polygon->addVertex(v);
    }
    rc.addPolygon(polygon);
    BOOST_REQUIRE(polygon->diceable());

    boost::shared_ptr<v3d::moya::MicroPolygonGrid> grid;
    BOOST_REQUIRE(polygon->dice(grid, rc));
    BOOST_REQUIRE(grid);

    const unsigned int last = grid->size() - 1;
    // the corner that leans keeps its own normal, and one diagonally across from it does not
    BOOST_TEST(grid->vertex(0, 0).normal().x > 0.7f);
    BOOST_TEST(grid->vertex(last, last).normal().x == 0.0f, boost::test_tools::tolerance(0.0001f));
    // between them the normal is neither, and is still a unit vector
    const glm::vec3 middle = grid->vertex(last / 2, last / 2).normal();
    BOOST_TEST(middle.x > 0.0f);
    BOOST_TEST(middle.x < 0.7f);
    BOOST_TEST(glm::length(middle) == 1.0f, boost::test_tools::tolerance(0.0001f));

    // Ng is the plane, everywhere on the grid
    BOOST_TEST((grid->vertex(0, 0).geometricNormal() == glm::vec3(0.0f, 0.0f, 1.0f)));
    BOOST_TEST((grid->vertex(last, last).geometricNormal() == glm::vec3(0.0f, 0.0f, 1.0f)));
}

/**
 * A split builds its pieces from intersection points, which carry no normal any more than they
 * carry a colour, so a piece takes the whole primitive's plane through place() - the same route
 * the colour takes. A surface large enough to split is therefore faceted per piece, and the
 * phase that gives the edge split an interpolating clip is what would change that.
 **/
BOOST_AUTO_TEST_CASE(render_context_split_carries_the_normal_test) {
    v3d::moya::RenderContext rc;
    rc.prepareWorld();

    boost::shared_ptr<v3d::moya::Polygon> polygon = boost::make_shared<v3d::moya::Polygon>();
    polygon->addVertex(vertex(-0.9f, -0.9f, 5.0f));
    polygon->addVertex(vertex(0.9f, -0.9f, 5.0f));
    polygon->addVertex(vertex(0.9f, 0.9f, 5.0f));
    polygon->addVertex(vertex(-0.9f, 0.9f, 5.0f));
    rc.addPolygon(polygon);

    // too large for one grid, so the second pass splits it rather than dicing it - and the
    // plane it hands its pieces is the one it was placed with
    BOOST_REQUIRE(!polygon->diceable());
    BOOST_TEST((polygon->normal() == glm::vec3(0.0f, 0.0f, 1.0f)));

    // the pieces reach the first pass again already placed, so they keep it: the render
    // terminates with every piece bucketed, which it could not do if a piece were re-measured
    // against the state of whatever came last
    rc.render();
    BOOST_TEST(rc.framebuffer()->primitiveCount() == 256u);
}

/**
 * An imager runs after the last bucket, over the finished frame, and gives a pixel nothing
 * was drawn into what the scene said it is worth.
 *
 * It is the same shader and the same runner talyn uses after its last ray, which is what
 * the coverage plane is for: without it a pixel the hider never reached and a black one
 * are the same number.
 **/
BOOST_AUTO_TEST_CASE(rendercontext_imager_test) {
    v3d::moya::RenderContext rc;
    rc.imageResolution(64, 48, 1.0f);
    rc.clipping(1.0f, 100.0f);
    rc.prepareWorld();

    v3d::render::offline::rib::ParameterList list;
    list.add("background",
        v3d::render::offline::rib::Declaration(
            v3d::render::offline::rib::Declaration::Storage::UNIFORM,
            v3d::render::offline::rib::Declaration::Type::COLOR, 1),
        { 0.15f, 0.25f, 0.45f }, std::vector<std::string>());
    rc.imager("background", list);

    boost::shared_ptr<v3d::moya::Polygon> polygon = boost::make_shared<v3d::moya::Polygon>();
    polygon->addVertex(vertex(-0.7f, -0.2f, 5.0f));
    polygon->addVertex(vertex(0.3f, -0.2f, 5.0f));
    polygon->addVertex(vertex(0.3f, 0.8f, 5.0f));
    polygon->addVertex(vertex(-0.7f, 0.8f, 5.0f));
    rc.addPolygon(polygon);
    rc.render();

    boost::shared_ptr<v3d::render::offline::FrameBuffer> planes = rc.framebuffer()->planes();
    // the quad covers raster x over [15.2, 39.2] and y over [4.8, 28.8], so this is inside
    // it and keeps the colour the hider wrote
    BOOST_CHECK_CLOSE(planes->value(v3d::moya::FrameBuffer::RED, 24, 16), 1.0f, 0.01f);
    // and this is outside it, where the imager is the whole of the pixel
    BOOST_CHECK_CLOSE(planes->value(v3d::moya::FrameBuffer::RED, 55, 40), 0.15f, 0.01f);
    BOOST_CHECK_CLOSE(planes->value(v3d::moya::FrameBuffer::BLUE, 55, 40), 0.45f, 0.01f);
    BOOST_CHECK_CLOSE(planes->value(v3d::moya::FrameBuffer::COVERAGE, 55, 40), 1.0f, 0.01f);
}
