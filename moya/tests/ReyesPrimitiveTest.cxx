/**
 * Vertical3D
 * Copyright(c) 2022 Joshua Farr(josh@farrcraft.com)
 **/

#include <moya/libmoya/Polygon.h>
#include <moya/libmoya/Vertex.h>
#include <moya/libmoya/reyes/Primitive.h>

#include <boost/test/unit_test.hpp>

#include <glm/gtc/matrix_transform.hpp>

namespace {

/**
 * A primitive is undiceable until RenderContext::addPolygon() has measured it against the grid
 * size. An unmeasured primitive is never assumed small enough to dice.
 **/
BOOST_AUTO_TEST_CASE(reyes_primitive_default_diceable_test) {
    const v3d::moya::reyes::Primitive primitive;

    BOOST_TEST(!primitive.diceable());
}

BOOST_AUTO_TEST_CASE(reyes_primitive_diceable_test) {
    v3d::moya::reyes::Primitive primitive;

    primitive.diceable(true);
    BOOST_TEST(primitive.diceable());

    primitive.diceable(false);
    BOOST_TEST(!primitive.diceable());
}

/**
 * Each primitive has its own diceable flag, and setting it on one does not change another.
 **/
BOOST_AUTO_TEST_CASE(reyes_primitive_flag_is_per_instance_test) {
    v3d::moya::reyes::Primitive first;
    v3d::moya::reyes::Primitive second;

    first.diceable(true);
    second.diceable(false);

    BOOST_TEST(first.diceable());
    BOOST_TEST(!second.diceable());
}

/**
 * A polygon is a primitive. The bound a caller reaches through the base is the polygon's own,
 * not the base's empty one.
 **/
BOOST_AUTO_TEST_CASE(reyes_primitive_polygon_is_a_primitive_test) {
    v3d::moya::Polygon polygon;
    v3d::moya::Vertex vertex;
    vertex.point(glm::vec3(2.0F, 3.0F, 4.0F));
    polygon.addVertex(vertex);

    v3d::moya::reyes::Primitive* primitive = &polygon;

    primitive->diceable(true);
    BOOST_TEST(primitive->diceable());
    BOOST_TEST((primitive->bound().max() == glm::vec3(2.0F, 3.0F, 4.0F)));
}

/**
 * A primitive carries the state it was submitted under: the object to eye transformation, the
 * colour that was current, and the plane it lies in. A split hands that state to its pieces. A
 * new primitive is not placed, and its normal is zero until it is.
 **/
BOOST_AUTO_TEST_CASE(reyes_primitive_place_test) {
    v3d::moya::reyes::Primitive primitive;

    BOOST_TEST(!primitive.placed());
    BOOST_TEST((primitive.normal() == glm::vec3(0.0F)));

    const glm::mat4x4 toEye = glm::translate(glm::mat4x4(1.0F), glm::vec3(0.0F, 0.0F, -4.0F));
    primitive.place(toEye, glm::vec3(1.0F, 0.0F, 0.0F), glm::vec3(0.0F, 1.0F, 0.0F),
        v3d::moya::Shading());

    BOOST_TEST(primitive.placed());
    BOOST_TEST((primitive.placement() == toEye));
    BOOST_TEST((primitive.color() == glm::vec3(1.0F, 0.0F, 0.0F)));
    BOOST_TEST((primitive.normal() == glm::vec3(0.0F, 1.0F, 0.0F)));
}

};  // namespace
