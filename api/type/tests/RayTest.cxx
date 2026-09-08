/**
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 **/

#include <boost/test/unit_test.hpp>

#include <glm/gtc/matrix_transform.hpp>

#include "../AABBox.h"
#include "../Ray.h"

BOOST_AUTO_TEST_CASE(ray_direction_test) {
    // the constructor normalises, so a distance along the ray is in the units its origin
    // and direction were given in
    v3d::type::Ray ray(glm::vec3(1.0f, 2.0f, 3.0f), glm::vec3(0.0f, 0.0f, 5.0f));
    BOOST_CHECK_CLOSE(ray.direction()[2], 1.0f, 0.01f);
    BOOST_CHECK_EQUAL(ray.origin()[0], 1.0f);

    glm::vec3 along = ray.point(2.0f);
    BOOST_CHECK_CLOSE(along[2], 5.0f, 0.01f);
    BOOST_CHECK_CLOSE(along[0], 1.0f, 0.01f);

    // a zero direction is left alone rather than dividing by zero
    v3d::type::Ray degenerate(glm::vec3(0.0f), glm::vec3(0.0f));
    BOOST_CHECK_EQUAL(degenerate.direction()[0], 0.0f);
    BOOST_CHECK_EQUAL(degenerate.direction()[2], 0.0f);
}

BOOST_AUTO_TEST_CASE(ray_transformed_test) {
    v3d::type::Ray ray(glm::vec3(0.0f, 0.0f, -10.0f), glm::vec3(0.0f, 0.0f, 1.0f));

    // the origin moves as a point and the direction as a vector, so a translation leaves
    // the direction alone
    glm::mat4 moved = glm::translate(glm::mat4(1.0f), glm::vec3(0.0f, 3.0f, 0.0f));
    v3d::type::Ray translated = ray.transformed(moved);
    BOOST_CHECK_CLOSE(translated.origin()[1], 3.0f, 0.01f);
    BOOST_CHECK_CLOSE(translated.direction()[2], 1.0f, 0.01f);

    // a scale is not renormalised away: the direction is halved, so the distance to a
    // given point is the same number it was in the space the ray came from
    glm::mat4 halved = glm::scale(glm::mat4(1.0f), glm::vec3(0.5f, 0.5f, 0.5f));
    v3d::type::Ray scaled = ray.transformed(halved);
    BOOST_CHECK_CLOSE(scaled.direction()[2], 0.5f, 0.01f);
    BOOST_CHECK_CLOSE(scaled.origin()[2], -5.0f, 0.01f);
    BOOST_CHECK_CLOSE(scaled.point(10.0f)[2], 0.0f, 0.01f);
}

BOOST_AUTO_TEST_CASE(ray_triangle_test) {
    const glm::vec3 a(-1.0f, -1.0f, 0.0f);
    const glm::vec3 b(1.0f, -1.0f, 0.0f);
    const glm::vec3 c(0.0f, 1.0f, 0.0f);

    float distance = 0.0f;

    // straight through the middle
    v3d::type::Ray hit(glm::vec3(0.0f, 0.0f, -5.0f), glm::vec3(0.0f, 0.0f, 1.0f));
    BOOST_CHECK_EQUAL(hit.intersects(a, b, c, &distance), true);
    BOOST_CHECK_CLOSE(distance, 5.0f, 0.01f);

    // from behind, which a modeller picks as readily as the front
    v3d::type::Ray behind(glm::vec3(0.0f, 0.0f, 5.0f), glm::vec3(0.0f, 0.0f, -1.0f));
    BOOST_CHECK_EQUAL(behind.intersects(a, b, c, &distance), true);
    BOOST_CHECK_CLOSE(distance, 5.0f, 0.01f);

    // outside the triangle but in its plane's way
    v3d::type::Ray beside(glm::vec3(3.0f, 0.0f, -5.0f), glm::vec3(0.0f, 0.0f, 1.0f));
    BOOST_CHECK_EQUAL(beside.intersects(a, b, c, nullptr), false);

    // pointing away from it - a half line, not a line
    v3d::type::Ray away(glm::vec3(0.0f, 0.0f, -5.0f), glm::vec3(0.0f, 0.0f, -1.0f));
    BOOST_CHECK_EQUAL(away.intersects(a, b, c, nullptr), false);

    // edge on, where the barycentric solution is meaningless
    v3d::type::Ray edgeOn(glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(1.0f, 0.0f, 0.0f));
    BOOST_CHECK_EQUAL(edgeOn.intersects(a, b, c, nullptr), false);
}

BOOST_AUTO_TEST_CASE(ray_triangle_barycentric_test) {
    // a right triangle on the z = 0 plane, so a weight reads straight off a coordinate
    const glm::vec3 a(0.0f, 0.0f, 0.0f);
    const glm::vec3 b(4.0f, 0.0f, 0.0f);
    const glm::vec3 c(0.0f, 4.0f, 0.0f);

    float distance = 0.0f;
    float u = 0.0f;
    float v = 0.0f;

    // a hit at b itself weighs b alone
    v3d::type::Ray corner(glm::vec3(4.0f, 0.0f, -2.0f), glm::vec3(0.0f, 0.0f, 1.0f));
    BOOST_CHECK_EQUAL(corner.intersects(a, b, c, &distance, &u, &v), true);
    BOOST_CHECK_CLOSE(distance, 2.0f, 0.01f);
    BOOST_CHECK_CLOSE(u, 1.0f, 0.01f);
    BOOST_CHECK_SMALL(v, 0.0001f);

    // (1, 2) is a + (1/4)(b - a) + (1/2)(c - a), so a carries the remaining quarter
    v3d::type::Ray inside(glm::vec3(1.0f, 2.0f, -2.0f), glm::vec3(0.0f, 0.0f, 1.0f));
    BOOST_CHECK_EQUAL(inside.intersects(a, b, c, &distance, &u, &v), true);
    BOOST_CHECK_CLOSE(u, 0.25f, 0.01f);
    BOOST_CHECK_CLOSE(v, 0.5f, 0.01f);
    BOOST_CHECK_CLOSE(1.0f - u - v, 0.25f, 0.01f);

    // the weights are of b and c in that order, not of the two the ray happens to be
    // nearer: swapping the triangle's last two corners swaps them
    BOOST_CHECK_EQUAL(inside.intersects(a, c, b, &distance, &u, &v), true);
    BOOST_CHECK_CLOSE(u, 0.5f, 0.01f);
    BOOST_CHECK_CLOSE(v, 0.25f, 0.01f);

    // a miss writes nothing, so what the caller had stands
    u = -1.0f;
    v = -1.0f;
    v3d::type::Ray beside(glm::vec3(5.0f, 5.0f, -2.0f), glm::vec3(0.0f, 0.0f, 1.0f));
    BOOST_CHECK_EQUAL(beside.intersects(a, b, c, &distance, &u, &v), false);
    BOOST_CHECK_EQUAL(u, -1.0f);
    BOOST_CHECK_EQUAL(v, -1.0f);

    // the overload without them answers what the overload with them answers
    v3d::type::Ray same(glm::vec3(1.0f, 2.0f, -2.0f), glm::vec3(0.0f, 0.0f, 1.0f));
    float plain = 0.0f;
    BOOST_CHECK_EQUAL(same.intersects(a, b, c, &plain), true);
    BOOST_CHECK_CLOSE(plain, 2.0f, 0.01f);
}

BOOST_AUTO_TEST_CASE(ray_box_test) {
    v3d::type::AABBox box;
    box.extents(glm::vec3(-1.0f, -1.0f, -1.0f), glm::vec3(1.0f, 1.0f, 1.0f));

    float distance = 0.0f;

    v3d::type::Ray hit(glm::vec3(0.0f, 0.0f, -5.0f), glm::vec3(0.0f, 0.0f, 1.0f));
    BOOST_CHECK_EQUAL(hit.intersects(box, &distance), true);
    BOOST_CHECK_CLOSE(distance, 4.0f, 0.01f);

    // a ray that starts inside hits at zero rather than missing
    v3d::type::Ray inside(glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(0.0f, 1.0f, 0.0f));
    BOOST_CHECK_EQUAL(inside.intersects(box, &distance), true);
    BOOST_CHECK_EQUAL(distance, 0.0f);

    // parallel to a pair of slabs and outside them
    v3d::type::Ray parallel(glm::vec3(0.0f, 5.0f, -5.0f), glm::vec3(0.0f, 0.0f, 1.0f));
    BOOST_CHECK_EQUAL(parallel.intersects(box, nullptr), false);

    // pointing away
    v3d::type::Ray away(glm::vec3(0.0f, 0.0f, -5.0f), glm::vec3(0.0f, 0.0f, -1.0f));
    BOOST_CHECK_EQUAL(away.intersects(box, nullptr), false);
}
