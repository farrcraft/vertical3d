#include <boost/test/unit_test.hpp>

#include "../3dtypes/Bound2D.h"

BOOST_AUTO_TEST_CASE( bound2d_test )
{
    v3D::Bound2D bound(2.0f, 5.0f, 10.0f, 20.0f);

    // test constructor & get position
    glm::vec2 position = bound.position();
    BOOST_CHECK_EQUAL(position[0], 2.0f);
    BOOST_CHECK_EQUAL(position[1], 5.0f);

    // test constructor & get size
    glm::vec2 size = bound.size();
    BOOST_CHECK_EQUAL(size[0], 10.0f);
    BOOST_CHECK_EQUAL(size[1], 20.0f);

    v3D::Bound2D bound2(position, size);

    // test 2nd constructor form
    glm::vec2 position2 = bound2.position();
    BOOST_CHECK_EQUAL((position == position2), true);

    // test 2nd constructor form
    glm::vec2 size2 = bound2.size();
    BOOST_CHECK_EQUAL((size == size2), true);

    // test expand
    bound.expand(10.0f);
    size = bound.size();
    BOOST_CHECK_EQUAL(size[0], 20.0f);
    BOOST_CHECK_EQUAL(size[1], 30.0f);
    position = bound.position();
    BOOST_CHECK_EQUAL(position[0], 12.0f);
    BOOST_CHECK_EQUAL(position[1], 15.0f);

    // test shrink
    bound.shrink(5.0f);
    size = bound.size();
    BOOST_CHECK_EQUAL(size[0], 15.0f);
    BOOST_CHECK_EQUAL(size[1],25.0f);
    position = bound.position();
    BOOST_CHECK_EQUAL(position[0], 17.0f);
    BOOST_CHECK_EQUAL(position[1], 20.0f);

    // test intersect
    glm::vec2 point(25.0f, 35.0f);
    bool inside = bound.intersect(point);
    BOOST_CHECK_EQUAL((inside == true), true);
    point = glm::vec2(10.0f, 5.0f);
    inside = bound.intersect(point);
    BOOST_CHECK_EQUAL((inside == false), true);

    // test addition
    bound += bound2;
    size = bound.size();
    BOOST_CHECK_EQUAL(size[0], 25.0f);
    BOOST_CHECK_EQUAL(size[1],45.0f);
    position = bound.position();
    BOOST_CHECK_EQUAL(position[0], 17.0f);
    BOOST_CHECK_EQUAL(position[1], 20.0f);
}
