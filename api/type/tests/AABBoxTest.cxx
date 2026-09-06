/**
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 **/

#include <boost/test/unit_test.hpp>

#include "../AABBox.h"

BOOST_AUTO_TEST_CASE(aabbox_test) {
    v3d::type::AABBox box;

    // set box minimum
    glm::vec3 minimum(5.0f, 4.0f, 3.0f);
    box.min(minimum);

    // test box minimum set/get
    glm::vec3 min_check = box.min();
    BOOST_CHECK_EQUAL((minimum == min_check), true);

    // set box maximum
    glm::vec3 maximum(10.0f, 15.0f, 20.0f);
    box.max(maximum);

    // test box maximum set/get
    glm::vec3 max_check = box.max();
    BOOST_CHECK_EQUAL((maximum == max_check), true);

    // test box origin
    // note origin() returns max - min, which is the box's size rather than its origin
    glm::vec3 origin = box.origin();
    BOOST_CHECK_EQUAL(origin[0], 5.0f);
    BOOST_CHECK_EQUAL(origin[1], 11.0f);
    BOOST_CHECK_EQUAL(origin[2], 17.0f);

    // test setting box extents
    glm::vec3 min_extent(3.0f, 9.0f, 11.0f);
    glm::vec3 max_extent(11.0f, 17.0f, 23.0f);
    box.extents(min_extent, max_extent);
    min_check = box.min();
    BOOST_CHECK_EQUAL((min_extent == min_check), true);
    max_check = box.max();
    BOOST_CHECK_EQUAL((max_extent == max_check), true);

    // test extending box
    glm::vec3 point(17.0f, 3.0f, 34.0f);
    box.extend(point);
    min_check = box.min();
    min_extent = glm::vec3(3.0f, 3.0f, 11.0f);
    BOOST_CHECK_EQUAL((min_extent == min_check), true);
    max_check = box.max();
    max_extent = glm::vec3(17.0f, 17.0f, 34.0f);
    BOOST_CHECK_EQUAL((max_extent == max_check), true);

    // test other branches of extend operation
    point = glm::vec3(1.0f, 27.0f, 7.0f);
    box.extend(point);
    min_check = box.min();
    min_extent = glm::vec3(1.0f, 3.0f, 7.0f);
    BOOST_CHECK_EQUAL((min_extent == min_check), true);
    max_check = box.max();
    max_extent = glm::vec3(17.0f, 27.0f, 34.0f);
    BOOST_CHECK_EQUAL((max_extent == max_check), true);

    // a point already inside the box leaves both extents alone
    box.extend(glm::vec3(5.0f, 5.0f, 15.0f));
    BOOST_CHECK_EQUAL((min_extent == box.min()), true);
    BOOST_CHECK_EQUAL((max_extent == box.max()), true);

    glm::vec3 vertices[8];
    box.vertices(vertices);
    glm::vec3 v0(1.0f, 27.0f, 34.0f);
    glm::vec3 v1(17.0f, 3.0f, 7.0f);
    glm::vec3 v2(17.0f, 27.0f, 7.0f);
    glm::vec3 v3(1.0f, 27.0f, 7.0f);
    glm::vec3 v4(17.0f, 3.0f, 34.0f);
    glm::vec3 v5(1.0f, 3.0f, 34.0f);
    BOOST_CHECK_EQUAL((v0 == vertices[0]), true);
    BOOST_CHECK_EQUAL((v1 == vertices[1]), true);
    BOOST_CHECK_EQUAL((v2 == vertices[2]), true);
    BOOST_CHECK_EQUAL((v3 == vertices[3]), true);
    BOOST_CHECK_EQUAL((v4 == vertices[4]), true);
    BOOST_CHECK_EQUAL((v5 == vertices[5]), true);
    BOOST_CHECK_EQUAL((min_extent == vertices[6]), true);
    BOOST_CHECK_EQUAL((max_extent == vertices[7]), true);
}
