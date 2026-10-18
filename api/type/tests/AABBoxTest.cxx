/**
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 **/

#include <api/type/geometry/AABBox.h>

#include <boost/test/unit_test.hpp>

namespace {

BOOST_AUTO_TEST_CASE(aabbox_test) {
    v3d::type::geometry::AABBox box;

    // set box minimum
    const glm::vec3 minimum(5.0F, 4.0F, 3.0F);
    box.min(minimum);

    // test box minimum set/get
    glm::vec3 min_check = box.min();
    BOOST_CHECK_EQUAL((minimum == min_check), true);

    // set box maximum
    const glm::vec3 maximum(10.0F, 15.0F, 20.0F);
    box.max(maximum);

    // test box maximum set/get
    glm::vec3 max_check = box.max();
    BOOST_CHECK_EQUAL((maximum == max_check), true);

    // test setting box extents
    glm::vec3 min_extent(3.0F, 9.0F, 11.0F);
    glm::vec3 max_extent(11.0F, 17.0F, 23.0F);
    box.extents(min_extent, max_extent);
    min_check = box.min();
    BOOST_CHECK_EQUAL((min_extent == min_check), true);
    max_check = box.max();
    BOOST_CHECK_EQUAL((max_extent == max_check), true);

    // test extending box
    glm::vec3 point(17.0F, 3.0F, 34.0F);
    box.extend(point);
    min_check = box.min();
    min_extent = glm::vec3(3.0F, 3.0F, 11.0F);
    BOOST_CHECK_EQUAL((min_extent == min_check), true);
    max_check = box.max();
    max_extent = glm::vec3(17.0F, 17.0F, 34.0F);
    BOOST_CHECK_EQUAL((max_extent == max_check), true);

    // test other branches of extend operation
    point = glm::vec3(1.0F, 27.0F, 7.0F);
    box.extend(point);
    min_check = box.min();
    min_extent = glm::vec3(1.0F, 3.0F, 7.0F);
    BOOST_CHECK_EQUAL((min_extent == min_check), true);
    max_check = box.max();
    max_extent = glm::vec3(17.0F, 27.0F, 34.0F);
    BOOST_CHECK_EQUAL((max_extent == max_check), true);

    // a point already inside the box leaves both extents alone
    box.extend(glm::vec3(5.0F, 5.0F, 15.0F));
    BOOST_CHECK_EQUAL((min_extent == box.min()), true);
    BOOST_CHECK_EQUAL((max_extent == box.max()), true);

    glm::vec3 vertices[8];
    box.vertices(vertices);
    const glm::vec3 v0(1.0F, 27.0F, 34.0F);
    const glm::vec3 v1(17.0F, 3.0F, 7.0F);
    const glm::vec3 v2(17.0F, 27.0F, 7.0F);
    const glm::vec3 v3(1.0F, 27.0F, 7.0F);
    const glm::vec3 v4(17.0F, 3.0F, 34.0F);
    const glm::vec3 v5(1.0F, 3.0F, 34.0F);
    BOOST_CHECK_EQUAL((v0 == vertices[0]), true);
    BOOST_CHECK_EQUAL((v1 == vertices[1]), true);
    BOOST_CHECK_EQUAL((v2 == vertices[2]), true);
    BOOST_CHECK_EQUAL((v3 == vertices[3]), true);
    BOOST_CHECK_EQUAL((v4 == vertices[4]), true);
    BOOST_CHECK_EQUAL((v5 == vertices[5]), true);
    BOOST_CHECK_EQUAL((min_extent == vertices[6]), true);
    BOOST_CHECK_EQUAL((max_extent == vertices[7]), true);
}

namespace {

v3d::type::geometry::AABBox box(const glm::vec3 & min, const glm::vec3 & max) {
    v3d::type::geometry::AABBox built;
    built.extents(min, max);
    return built;
}

};  // namespace

/**
 * Two boxes overlap unless they are apart on at least one axis, and sharing exactly a face
 * counts as overlapping.
 **/
BOOST_AUTO_TEST_CASE(aabbox_overlaps_test) {
    const v3d::type::geometry::AABBox unit = box(glm::vec3(0.0F), glm::vec3(1.0F));

    // apart on each axis alone
    for (int axis = 0; axis < 3; axis++) {
        glm::vec3 offset(0.0F);
        offset[axis] = 2.0F;
        BOOST_TEST(!unit.overlaps(box(offset, offset + glm::vec3(1.0F))));
        BOOST_TEST(!unit.overlaps(box(-offset, -offset + glm::vec3(1.0F))));
    }

    // overlapping, from both sides
    const v3d::type::geometry::AABBox shifted = box(glm::vec3(0.5F), glm::vec3(1.5F));
    BOOST_TEST(unit.overlaps(shifted));
    BOOST_TEST(shifted.overlaps(unit));

    // one inside the other
    const v3d::type::geometry::AABBox inner = box(glm::vec3(0.25F), glm::vec3(0.75F));
    BOOST_TEST(unit.overlaps(inner));
    BOOST_TEST(inner.overlaps(unit));

    // sharing exactly a face
    BOOST_TEST(unit.overlaps(box(glm::vec3(1.0F, 0.0F, 0.0F), glm::vec3(2.0F, 1.0F, 1.0F))));
}

BOOST_AUTO_TEST_CASE(aabbox_contains_test) {
    const v3d::type::geometry::AABBox unit = box(glm::vec3(0.0F), glm::vec3(1.0F));

    BOOST_TEST(unit.contains(glm::vec3(0.5F)));
    BOOST_TEST(unit.contains(glm::vec3(1.0F, 0.0F, 1.0F)));
    BOOST_TEST(!unit.contains(glm::vec3(0.5F, 1.5F, 0.5F)));
    BOOST_TEST(!unit.contains(glm::vec3(0.5F, 0.5F, -0.5F)));
}

};  // namespace
