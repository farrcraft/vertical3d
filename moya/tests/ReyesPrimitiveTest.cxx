/**
 * Vertical3D
 * Copyright(c) 2022 Joshua Farr(josh@farrcraft.com)
 **/

#include <boost/test/unit_test.hpp>

#include "../libmoya/Polygon.h"
#include "../libmoya/ReyesPrimitive.h"

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
