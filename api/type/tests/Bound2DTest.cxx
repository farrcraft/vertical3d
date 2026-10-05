/**
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 **/

#include <api/type/geometry/Bound2D.h>

#include <boost/test/unit_test.hpp>

BOOST_AUTO_TEST_CASE(bound2d_test) {
    v3d::type::geometry::Bound2D bound(2.0f, 5.0f, 10.0f, 20.0f);

    // test constructor & get position
    glm::vec2 position = bound.position();
    BOOST_CHECK_EQUAL(position[0], 2.0f);
    BOOST_CHECK_EQUAL(position[1], 5.0f);

    // test constructor & get size
    glm::vec2 size = bound.size();
    BOOST_CHECK_EQUAL(size[0], 10.0f);
    BOOST_CHECK_EQUAL(size[1], 20.0f);

    v3d::type::geometry::Bound2D bound2(position, size);

    // test 2nd constructor form
    glm::vec2 position2 = bound2.position();
    BOOST_CHECK_EQUAL((position == position2), true);

    // test 2nd constructor form
    glm::vec2 size2 = bound2.size();
    BOOST_CHECK_EQUAL((size == size2), true);

    // test contains
    BOOST_CHECK_EQUAL(bound.contains(glm::vec2(7.0f, 15.0f)), true);
    BOOST_CHECK_EQUAL(bound.contains(glm::vec2(1.0f, 5.0f)), false);
}

/**
 * Two bounds overlap unless they are apart on at least one axis, and sharing exactly an edge
 * counts as overlapping, the same closed edge contains() uses.
 **/
BOOST_AUTO_TEST_CASE(bound2d_overlaps_test) {
    const v3d::type::geometry::Bound2D bound(0.0f, 0.0f, 10.0f, 10.0f);

    // apart on x only, and on y only
    BOOST_TEST(!bound.overlaps(v3d::type::geometry::Bound2D(11.0f, 0.0f, 5.0f, 5.0f)));
    BOOST_TEST(!bound.overlaps(v3d::type::geometry::Bound2D(-6.0f, 2.0f, 5.0f, 5.0f)));
    BOOST_TEST(!bound.overlaps(v3d::type::geometry::Bound2D(2.0f, 11.0f, 5.0f, 5.0f)));
    BOOST_TEST(!bound.overlaps(v3d::type::geometry::Bound2D(2.0f, -6.0f, 5.0f, 5.0f)));

    // overlapping a corner, which is the answer from both sides
    const v3d::type::geometry::Bound2D corner(8.0f, 8.0f, 5.0f, 5.0f);
    BOOST_TEST(bound.overlaps(corner));
    BOOST_TEST(corner.overlaps(bound));

    // one inside the other
    BOOST_TEST(bound.overlaps(v3d::type::geometry::Bound2D(2.0f, 2.0f, 1.0f, 1.0f)));
    BOOST_TEST(v3d::type::geometry::Bound2D(2.0f, 2.0f, 1.0f, 1.0f).overlaps(bound));

    // sharing exactly an edge
    BOOST_TEST(bound.overlaps(v3d::type::geometry::Bound2D(10.0f, 0.0f, 5.0f, 5.0f)));
}

BOOST_AUTO_TEST_CASE(bound2d_contains_test) {
    const v3d::type::geometry::Bound2D bound(0.0f, 0.0f, 10.0f, 10.0f);

    BOOST_TEST(bound.contains(glm::vec2(5.0f, 5.0f)));
    BOOST_TEST(bound.contains(glm::vec2(10.0f, 0.0f)));
    BOOST_TEST(!bound.contains(glm::vec2(10.5f, 5.0f)));
    BOOST_TEST(!bound.contains(glm::vec2(5.0f, -0.5f)));
}
