/**
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 **/

#include <api/type/geometry/Bound2D.h>

#include <boost/test/unit_test.hpp>

namespace {

BOOST_AUTO_TEST_CASE(bound2d_test) {
    const v3d::type::geometry::Bound2D bound({.position = glm::vec2(2.0F, 5.0F), .size = glm::vec2(10.0F, 20.0F)});

    // test constructor & get position
    glm::vec2 position = bound.position();
    BOOST_CHECK_EQUAL(position[0], 2.0F);
    BOOST_CHECK_EQUAL(position[1], 5.0F);

    // test constructor & get size
    glm::vec2 size = bound.size();
    BOOST_CHECK_EQUAL(size[0], 10.0F);
    BOOST_CHECK_EQUAL(size[1], 20.0F);

    const v3d::type::geometry::Bound2D bound2({.position = position, .size = size});

    // a bound built from another's position and size reports them back
    const glm::vec2 position2 = bound2.position();
    BOOST_CHECK_EQUAL((position == position2), true);

    const glm::vec2 size2 = bound2.size();
    BOOST_CHECK_EQUAL((size == size2), true);

    // test contains
    BOOST_CHECK_EQUAL(bound.contains(glm::vec2(7.0F, 15.0F)), true);
    BOOST_CHECK_EQUAL(bound.contains(glm::vec2(1.0F, 5.0F)), false);
}

/**
 * Two bounds overlap unless they are apart on at least one axis, and sharing exactly an edge
 * counts as overlapping, the same closed edge contains() uses.
 **/
BOOST_AUTO_TEST_CASE(bound2d_overlaps_test) {
    const v3d::type::geometry::Bound2D bound({.position = glm::vec2(0.0F, 0.0F), .size = glm::vec2(10.0F, 10.0F)});

    // apart on x only, and on y only
    BOOST_TEST(!bound.overlaps(v3d::type::geometry::Bound2D({.position = glm::vec2(11.0F, 0.0F), .size = glm::vec2(5.0F, 5.0F)})));
    BOOST_TEST(!bound.overlaps(v3d::type::geometry::Bound2D({.position = glm::vec2(-6.0F, 2.0F), .size = glm::vec2(5.0F, 5.0F)})));
    BOOST_TEST(!bound.overlaps(v3d::type::geometry::Bound2D({.position = glm::vec2(2.0F, 11.0F), .size = glm::vec2(5.0F, 5.0F)})));
    BOOST_TEST(!bound.overlaps(v3d::type::geometry::Bound2D({.position = glm::vec2(2.0F, -6.0F), .size = glm::vec2(5.0F, 5.0F)})));

    // overlapping a corner, which is the answer from both sides
    const v3d::type::geometry::Bound2D corner({.position = glm::vec2(8.0F, 8.0F), .size = glm::vec2(5.0F, 5.0F)});
    BOOST_TEST(bound.overlaps(corner));
    BOOST_TEST(corner.overlaps(bound));

    // one inside the other
    BOOST_TEST(bound.overlaps(v3d::type::geometry::Bound2D({.position = glm::vec2(2.0F, 2.0F), .size = glm::vec2(1.0F, 1.0F)})));
    BOOST_TEST(v3d::type::geometry::Bound2D({.position = glm::vec2(2.0F, 2.0F), .size = glm::vec2(1.0F, 1.0F)}).overlaps(bound));

    // sharing exactly an edge
    BOOST_TEST(bound.overlaps(v3d::type::geometry::Bound2D({.position = glm::vec2(10.0F, 0.0F), .size = glm::vec2(5.0F, 5.0F)})));
}

BOOST_AUTO_TEST_CASE(bound2d_contains_test) {
    const v3d::type::geometry::Bound2D bound({.position = glm::vec2(0.0F, 0.0F), .size = glm::vec2(10.0F, 10.0F)});

    BOOST_TEST(bound.contains(glm::vec2(5.0F, 5.0F)));
    BOOST_TEST(bound.contains(glm::vec2(10.0F, 0.0F)));
    BOOST_TEST(!bound.contains(glm::vec2(10.5F, 5.0F)));
    BOOST_TEST(!bound.contains(glm::vec2(5.0F, -0.5F)));
}

};  // namespace
