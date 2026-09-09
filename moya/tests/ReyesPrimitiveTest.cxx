/**
 * Vertical3D
 * Copyright(c) 2022 Joshua Farr(josh@farrcraft.com)
 **/

#include <moya/libmoya/Polygon.h>
#include <moya/libmoya/ReyesPrimitive.h>

#include <boost/test/unit_test.hpp>

#include <glm/gtc/matrix_transform.hpp>

/**
 * A primitive is undiceable until the first pass has measured it against the grid size, so an
 * unmeasured one is routed through that measurement rather than assumed small enough to skip
 * it.
 **/
BOOST_AUTO_TEST_CASE(reyes_primitive_default_diceable_test) {
    v3d::moya::ReyesPrimitive primitive;

    BOOST_TEST(!primitive.diceable());
}

BOOST_AUTO_TEST_CASE(reyes_primitive_diceable_test) {
    v3d::moya::ReyesPrimitive primitive;

    primitive.diceable(true);
    BOOST_TEST(primitive.diceable());

    primitive.diceable(false);
    BOOST_TEST(!primitive.diceable());
}

/**
 * The flag is per primitive, so splitting one does not decide for its neighbours.
 **/
BOOST_AUTO_TEST_CASE(reyes_primitive_flag_is_per_instance_test) {
    v3d::moya::ReyesPrimitive first;
    v3d::moya::ReyesPrimitive second;

    first.diceable(true);
    second.diceable(false);

    BOOST_TEST(first.diceable());
    BOOST_TEST(!second.diceable());
}

/**
 * A polygon is a primitive, and the bound a caller reaches through the base is the polygon's
 * own rather than the base's empty one.
 **/
BOOST_AUTO_TEST_CASE(reyes_primitive_polygon_is_a_primitive_test) {
    v3d::moya::Polygon polygon;
    v3d::moya::Vertex vertex;
    vertex.point(glm::vec3(2.0f, 3.0f, 4.0f));
    polygon.addVertex(vertex);

    v3d::moya::ReyesPrimitive* primitive = &polygon;

    primitive->diceable(true);
    BOOST_TEST(primitive->diceable());
    BOOST_TEST((primitive->bound().max() == glm::vec3(2.0f, 3.0f, 4.0f)));
}

/**
 * A primitive carries the state it was submitted under so that a split can hand it to its
 * pieces: the object to eye transformation, the colour that was current, and the plane it lies
 * in. Nothing has placed a fresh one, and its normal names no plane until something does.
 **/
BOOST_AUTO_TEST_CASE(reyes_primitive_place_test) {
    v3d::moya::ReyesPrimitive primitive;

    BOOST_TEST(!primitive.placed());
    BOOST_TEST((primitive.normal() == glm::vec3(0.0f)));

    const glm::mat4x4 toEye = glm::translate(glm::mat4x4(1.0f), glm::vec3(0.0f, 0.0f, -4.0f));
    primitive.place(toEye, glm::vec3(1.0f, 0.0f, 0.0f), glm::vec3(0.0f, 1.0f, 0.0f));

    BOOST_TEST(primitive.placed());
    BOOST_TEST((primitive.placement() == toEye));
    BOOST_TEST((primitive.color() == glm::vec3(1.0f, 0.0f, 0.0f)));
    BOOST_TEST((primitive.normal() == glm::vec3(0.0f, 1.0f, 0.0f)));
}
